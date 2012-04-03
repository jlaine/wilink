/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QDesktopServices>
#include <QSet>
#include <QTimer>

#include "QDjango.h"
#include "QXmppClient.h"
#include "QXmppPresence.h"
#include "QXmppShareDatabase.h"
#include "QXmppShareManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "application.h"
#include "client.h"
#include "roster.h"
#include "shares.h"

Q_IMPORT_PLUGIN(share_filesystem)

static QXmppShareDatabase *globalDatabase = 0;
static const int parallelDownloadLimit = 2;

static QUrl locationToUrl(const QXmppShareLocation& loc)
{
    QUrl url;
    url.setScheme("share");
    url.setPath(QString::fromLatin1(loc.jid().toUtf8().toHex()) + "/" + loc.node());
    return url;
}

static QXmppShareLocation urlToLocation(const QUrl &url)
{
    QXmppShareLocation loc;
    const QString path = url.path();
    const int slashPos = path.indexOf('/');
    if (slashPos > 0) {
        loc.setJid(QString::fromUtf8(QByteArray::fromHex(path.left(slashPos).toAscii())));
        loc.setNode(path.mid(slashPos + 1));
    }
    return loc;
}

static void closeDatabase()
{
    delete globalDatabase;
}

static void copy(QXmppShareItem *oldChild, const QXmppShareItem *newChild)
{
    oldChild->setFileDate(newChild->fileDate());
    oldChild->setFileHash(newChild->fileHash());
    oldChild->setFileSize(newChild->fileSize());
    oldChild->setLocations(newChild->locations());
    oldChild->setName(newChild->name());
    oldChild->setPopularity(newChild->popularity());
    oldChild->setType(newChild->type());
}

static void totals(const QXmppShareItem *item, qint64 &bytes, qint64 &files)
{
    for (int i = 0; i < item->size(); ++i) {
        const QXmppShareItem *child = item->child(i);
        if (child->type() == QXmppShareItem::FileItem) {
            bytes += child->fileSize();
            files++;
        } else {
            totals(child, bytes, files);
        }
    }
}

class ShareModelPrivate
{
public:
    ShareModelPrivate(ShareModel *qq);
    bool canDownload(const QXmppShareItem *item) const;
    void setShareClient(ChatClient *shareClient);
    QXmppShareManager *shareManager();

    ChatClient *client;
    bool connected;
    QString filter;
    QString requestId;
    QString rootJid;
    QString rootNode;
    ChatClient *shareClient;
    QString shareServer;
    QTimer *timer;
    ShareQueueModel *queueModel;

private:
    ShareModel *q;
};

ShareModelPrivate::ShareModelPrivate(ShareModel *qq)
    : client(0),
      connected(false),
      shareClient(0),
      q(qq)
{
    queueModel = new ShareQueueModel(q);
}

bool ShareModelPrivate::canDownload(const QXmppShareItem *item) const
{
    if (item->locations().isEmpty())
        return false;

    // don't download from self
    foreach (const QXmppShareLocation &location, item->locations()) {
        if (location.jid() == shareClient->configuration().jid())
            return false;
    }

    return !queueModel->contains(*item);
}

void ShareModelPrivate::setShareClient(ChatClient *newClient)
{
    if (newClient == shareClient)
        return;

    // delete old share client
    if (shareClient) {
        shareClient->disconnect(q, SLOT(_q_presenceReceived(QXmppPresence)));
        shareClient->disconnect(q, SLOT(_q_serverChanged(QString)));
        if (shareClient != client)
            shareClient->deleteLater();
    }

    // set new share client
    shareClient = newClient;
    if (shareClient) {
        bool check;
        Q_UNUSED(check);

        check = q->connect(shareClient, SIGNAL(presenceReceived(QXmppPresence)),
                           q, SLOT(_q_presenceReceived(QXmppPresence)));
        Q_ASSERT(check);

        check = q->connect(shareClient, SIGNAL(shareServerChanged(QString)),
                           q, SLOT(_q_serverChanged(QString)));
        Q_ASSERT(check);

        // if we already know the server, register with it
        const QString server = shareClient->shareServer();
        if (!server.isEmpty())
            q->_q_serverChanged(server);
    }
}

QXmppShareManager *ShareModelPrivate::shareManager()
{
    if (shareClient)
        return shareClient->findExtension<QXmppShareManager>();
    else
        return 0;
}

ShareModel::ShareModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    d = new ShareModelPrivate(this);
    rootItem = new QXmppShareItem(QXmppShareItem::CollectionItem);

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(CanDownloadRole, "canDownload");
    roleNames.insert(IsDirRole, "isDir");
    roleNames.insert(JidRole, "jid");
    roleNames.insert(NameRole, "name");
    roleNames.insert(NodeRole, "node");
    roleNames.insert(PopularityRole, "popularity");
    roleNames.insert(SizeRole, "size");
    setRoleNames(roleNames);

    // add timer
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    d->timer->setInterval(100);
    connect(d->timer, SIGNAL(timeout()), this, SLOT(refresh()));

    connect(database(), SIGNAL(directoryChanged(QString)),
            this, SIGNAL(shareUrlChanged()));
}

ShareModel::~ShareModel()
{
    delete rootItem;
    delete d;
}

ChatClient *ShareModel::client() const
{
    return d->client;
}

void ShareModel::setClient(ChatClient *client)
{
    bool check;
    Q_UNUSED(check);

    if (client != d->client) {
        d->client = client;

        check = connect(d->client, SIGNAL(disconnected()),
                        this, SLOT(_q_disconnected()));
        Q_ASSERT(check);

        d->setShareClient(d->client);
        emit clientChanged(d->client);
    }
}

QString ShareModel::filter() const
{
    return d->filter;
}

void ShareModel::setFilter(const QString &filter)
{
    if (filter != d->filter) {
        d->filter = filter;
        d->timer->start();
        emit filterChanged(d->filter);
    }
}

bool ShareModel::isBusy() const
{
    return !d->requestId.isEmpty();
}

bool ShareModel::isConnected() const
{
    return d->connected;
}

ShareQueueModel* ShareModel::queue() const
{
    return d->queueModel;
}

QString ShareModel::rootJid() const
{
    return d->rootJid;
}

void ShareModel::setRootJid(const QString &rootJid)
{
    if (rootJid != d->rootJid) {
        d->rootJid = rootJid;
        d->timer->start();
        emit rootJidChanged(d->rootJid);
    }
}

QString ShareModel::rootNode() const
{
    return d->rootNode;
}

void ShareModel::setRootNode(const QString &rootNode)
{
    if (rootNode != d->rootNode) {
        d->rootNode = rootNode;
        d->timer->start();
        emit rootNodeChanged(d->rootNode);
    }
}

QString ShareModel::shareServer() const
{
    return d->shareManager() ? d->shareServer : QString();
}

QUrl ShareModel::shareUrl() const
{
    return QUrl::fromLocalFile(database()->directory());
}

void ShareModel::clear()
{
    rootItem->clearChildren();
    reset();
}

int ShareModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex ShareModel::createIndex(QXmppShareItem *item, int column) const
{
    if (item && item != rootItem)
        return QAbstractItemModel::createIndex(item->row(), column, item);
    else
        return QModelIndex();
}

QVariant ShareModel::data(const QModelIndex &index, int role) const
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == CanDownloadRole)
        return d->canDownload(item);
    else if (role == IsDirRole)
        return item->type() == QXmppShareItem::CollectionItem;
    else if (role == JidRole) {
        if (item->locations().isEmpty())
            return QString();
        else
            return item->locations().first().jid();
    }
    else if (role == NameRole)
        return item->name();
    else if (role == NodeRole) {
        if (item->locations().isEmpty())
            return QString();
        else
            return item->locations().first().node();
    }
    else if (role == PopularityRole)
        return item->popularity();
    else if (role == SizeRole)
        return item->fileSize();
    return QVariant();
}

QXmppShareDatabase *ShareModel::database() const
{
    bool check;
    Q_UNUSED(check);

    if (!globalDatabase) {
        // initialise database
        const QString databaseName = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).filePath("database.sqlite");
        QSqlDatabase sharesDb = QSqlDatabase::addDatabase("QSQLITE");
        sharesDb.setDatabaseName(databaseName);
        check = sharesDb.open();
        Q_ASSERT(check);

        QDjango::setDatabase(sharesDb);
        // drop wiLink <= 0.9.4 table
        sharesDb.exec("DROP TABLE files");

        // create shares database
        globalDatabase = new QXmppShareDatabase;
        check = connect(wApp->settings(), SIGNAL(sharesDirectoriesChanged(QVariantList)),
                        this, SLOT(_q_settingsChanged()));
        Q_ASSERT(check);
        check = connect(wApp->settings(), SIGNAL(sharesLocationChanged(QString)),
                        this, SLOT(_q_settingsChanged()));
        Q_ASSERT(check);
        _q_settingsChanged();
        qAddPostRoutine(closeDatabase);
    }
    return globalDatabase;
}

void ShareModel::download(int row)
{
    if (row < 0 || row > rootItem->size() - 1)
        return;

    QXmppShareItem *item = rootItem->child(row);
    if (d->canDownload(item)) {
        d->queueModel->add(*item, d->filter);
        emit dataChanged(createIndex(item), createIndex(item));
    }
}

QModelIndex ShareModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());

    QXmppShareItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(childItem);
    else
        return QModelIndex();
}

QModelIndex ShareModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareItem *childItem = static_cast<QXmppShareItem*>(index.internalPointer());
    QXmppShareItem *parentItem = childItem->parent();

    return createIndex(parentItem);
}

void ShareModel::refresh()
{
    QXmppShareManager *shareManager = d->shareManager();
    if (!shareManager || d->rootJid.isEmpty())
        return;

    // browse files
    clear();
    d->requestId = shareManager->search(QXmppShareLocation(d->rootJid, d->rootNode), 1, d->filter);
    emit isBusyChanged();
}

int ShareModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());
    return parentItem->size();
}

void ShareModel::_q_disconnected()
{
    // if we are using a slave client and the main client
    // disconnects, kill the slave
    if (d->client != d->shareClient && sender() == d->client)
        d->setShareClient(d->client);

    // clear model and any pending request
    if (d->connected) {
        d->connected = false;
        emit isConnectedChanged();
    }
    clear();
    d->requestId.clear();
    emit isBusyChanged();
}

void ShareModel::_q_presenceReceived(const QXmppPresence &presence)
{
    bool check;
    Q_UNUSED(check);

    Q_ASSERT(d->shareClient);
    if (d->shareServer.isEmpty() || presence.from() != d->shareServer)
        return;

    // find shares extension
    QXmppElement shareExtension;
    foreach (const QXmppElement &extension, presence.extensions())
    {
        if (extension.attribute("xmlns") == ns_shares)
        {
            shareExtension = extension;
            break;
        }
    }
    if (shareExtension.attribute("xmlns") != ns_shares)
        return;

    if (presence.type() == QXmppPresence::Available)
    {
        // configure transfer manager
        const QString forceProxy = shareExtension.firstChildElement("force-proxy").value();
        QXmppTransferManager *transferManager = d->shareClient->findExtension<QXmppTransferManager>();
        if (forceProxy == QLatin1String("1") && !transferManager->proxyOnly()) {
            qDebug("Shares forcing SOCKS5 proxy");
            transferManager->setProxyOnly(true);
        }

        // add share manager
        QXmppShareManager *shareManager = d->shareClient->findExtension<QXmppShareManager>();
        if (!shareManager) {
            shareManager = new QXmppShareManager(d->shareClient, database());
            d->shareClient->addExtension(shareManager);

            check = connect(shareManager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                            this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
            Q_ASSERT(check);
        }
        d->queueModel->setManager(shareManager);

        if (!d->connected) {
            d->connected = true;
            emit isConnectedChanged();
        }
    }
    else if (presence.type() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString newDomain = shareExtension.firstChildElement("domain").value();
        const QString newJid = d->client->configuration().user() + '@' + newDomain;

        // avoid redirect loop
        if (d->shareClient != d->client ||
            newDomain == d->client->configuration().domain()) {
            qWarning("Shares not redirecting to domain %s", qPrintable(newDomain));
            return;
        }

        // replace client
        ChatClient *newClient = new ChatClient(this);
        newClient->setLogger(d->client->logger());

        check = connect(newClient, SIGNAL(disconnected()),
                        this, SLOT(_q_disconnected()));
        Q_ASSERT(check);

        d->setShareClient(newClient);

        // reconnect to another server
        qDebug("Shares redirecting to %s", qPrintable(newDomain));
        newClient->connectToServer(newJid, d->client->configuration().password());
    }
}

void ShareModel::_q_serverChanged(const QString &server)
{
    Q_ASSERT(d->shareClient);

    // update server
    d->shareServer = server;
    emit shareServerChanged(d->shareServer);
    if (d->shareServer.isEmpty())
        return;

    qDebug("Shares registering with %s", qPrintable(d->shareServer));

    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    VCard card;
    card.setJid(jidToBareJid(d->client->jid()));

    QXmppElement nickName;
    nickName.setTagName("nickName");
    nickName.setValue(card.name());
    x.appendChild(nickName);

    QXmppPresence presence;
    presence.setTo(d->shareServer);
    presence.setExtensions(x);
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNone);
    d->shareClient->sendPacket(presence);
}

void ShareModel::_q_searchReceived(const QXmppShareSearchIq &shareIq)
{
    // filter requests
    if (shareIq.from() != d->rootJid ||
        shareIq.type() == QXmppIq::Get ||
        (shareIq.type() == QXmppIq::Set && !d->rootNode.isEmpty()) ||
        (shareIq.type() != QXmppIq::Set && shareIq.id() != d->requestId))
        return;

    if (shareIq.type() == QXmppIq::Error)
    {
        removeRows(0, rootItem->size());
    } else {
        QXmppShareItem *newItem = (QXmppShareItem*)&shareIq.collection();

        // update own data
        copy(rootItem, newItem);

        // update children
        QList<QXmppShareItem*> removed = rootItem->children();
        for (int newRow = 0; newRow < newItem->size(); newRow++) {
            QXmppShareItem *newChild = newItem->child(newRow);
            QXmppShareItem *oldChild = 0;
            foreach (QXmppShareItem *ptr, removed) {
                if (ptr->locations() == newChild->locations()) {
                    oldChild = ptr;
                    break;
                }
            }

            if (oldChild) {
                // update existing child
                const int oldRow = oldChild->row();
                if (oldRow != newRow)
                {
                    beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
                    rootItem->moveChild(oldRow, newRow);
                    endMoveRows();
                }

                // update data
                copy(oldChild, newChild);
                emit dataChanged(createIndex(oldChild), createIndex(oldChild));

                removed.removeAll(oldChild);
            } else {
                // insert new child
                beginInsertRows(QModelIndex(), newRow, newRow);
                rootItem->insertChild(newRow, *newChild);
                endInsertRows();
            }
        }

        // remove old children
        foreach (QXmppShareItem *oldChild, removed) {
            beginRemoveRows(QModelIndex(), oldChild->row(), oldChild->row());
            rootItem->removeChild(oldChild);
            endRemoveRows();
        }
    }

    // notify busy change
    if (shareIq.id() == d->requestId) {
        d->requestId.clear();
        emit isBusyChanged();
    }
}

void ShareModel::_q_settingsChanged() const
{
    QStringList dirs;
    foreach (const QVariant &dir, wApp->settings()->sharesDirectories())
        dirs << dir.toUrl().toLocalFile();

    globalDatabase->setDirectory(wApp->settings()->sharesLocation());
    globalDatabase->setMappedDirectories(dirs);
}

class ShareQueueItem : public ChatModelItem
{
public:
    ShareQueueItem();
    QXmppShareItem *nextFile(QXmppShareItem *item = 0) const;

    QString requestId;
    QXmppShareItem shareItem;

    bool aborted;
    QSet<QXmppShareItem*> done;
    QMap<QXmppShareItem*, QXmppShareTransfer*> transfers;

    qint64 doneBytes;
    qint64 doneFiles;
    qint64 totalBytes;
    qint64 totalFiles;
};

ShareQueueItem::ShareQueueItem()
    : aborted(false),
    doneBytes(0),
    doneFiles(0),
    totalBytes(0),
    totalFiles(0)
{
}

QXmppShareItem *ShareQueueItem::nextFile(QXmppShareItem *item) const
{
    if (aborted)
        return 0;

    if (!item)
        item = (QXmppShareItem*)&shareItem;

    if (item->type() == QXmppShareItem::FileItem &&
        !done.contains(item) &&
        !transfers.contains(item))
        return item;

    foreach (QXmppShareItem *child, item->children()) {
        Q_ASSERT(child);
        QXmppShareItem *file = nextFile(child);
        if (file)
            return file;
    }

    return 0;
}

class ShareQueueModelPrivate
{
public:
    ShareQueueModelPrivate(ShareQueueModel *qq);
    void process();
    qint64 transfers() const;

    QXmppShareManager *manager;
    QTimer *timer;

private:
    ShareQueueModel *q;
};

ShareQueueModelPrivate::ShareQueueModelPrivate(ShareQueueModel *qq)
    : manager(0),
    timer(0),
    q(qq)
{
}

qint64 ShareQueueModelPrivate::transfers() const
{
    qint64 transfers = 0;
    foreach (ChatModelItem *ptr, q->rootItem->children) {
        ShareQueueItem *item = static_cast<ShareQueueItem*>(ptr);
        transfers += item->transfers.size();
    }
    return transfers;
}

void ShareQueueModelPrivate::process()
{
    Q_ASSERT(manager);

    // check how many downloads are active
    qint64 activeDownloads = transfers();
    if (activeDownloads >= parallelDownloadLimit)
        return;

    // start downloads
    foreach (ChatModelItem *ptr, q->rootItem->children) {
        ShareQueueItem *item = static_cast<ShareQueueItem*>(ptr);

        while (activeDownloads < parallelDownloadLimit) {
            // find next item
            QXmppShareItem *file = item->nextFile();
            if (!file)
                break;

            QXmppShareTransfer *transfer = manager->get(*file);
            if (transfer) {
                bool check;
                Q_UNUSED(check);

                check = q->connect(transfer, SIGNAL(finished()),
                                   q, SLOT(_q_transferFinished()));
                Q_ASSERT(check);

                item->transfers.insert(file, transfer);
                activeDownloads++;

                // start periodic refresh
                if (!timer->isActive())
                    timer->start();
            } else {
                item->done.insert(file);
            }
            emit q->dataChanged(q->createIndex(item), q->createIndex(item));
        }
    }
}

ShareQueueModel::ShareQueueModel(QObject *parent)
    : ChatModel(parent)
{
    bool check;
    Q_UNUSED(check);

    d = new ShareQueueModelPrivate(this);

    // timer to refresh progress
    d->timer = new QTimer(this);
    d->timer->setInterval(500);
    check = connect(d->timer, SIGNAL(timeout()),
                    this, SLOT(_q_refresh()));
    Q_ASSERT(check);

    // set role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(IsDirRole, "isDir");
    names.insert(NodeRole, "node");
    names.insert(SpeedRole, "speed");
    names.insert(DoneBytesRole, "doneBytes");
    names.insert(DoneFilesRole, "doneFiles");
    names.insert(TotalBytesRole, "totalBytes");
    names.insert(TotalFilesRole, "totalFiles");
    setRoleNames(names);
}

ShareQueueModel::~ShareQueueModel()
{
    delete d;
}

void ShareQueueModel::add(const QXmppShareItem &shareItem, const QString &filter)
{
    if (!d->manager || shareItem.locations().isEmpty())
        return;

    ShareQueueItem *item = new ShareQueueItem;
    copy(&item->shareItem, &shareItem);

    if (item->shareItem.type() == QXmppShareItem::CollectionItem) {
        item->requestId = d->manager->search(item->shareItem.locations().first(), 0, filter);
    } else {
        item->totalBytes = item->shareItem.fileSize();
        item->totalFiles = 1;
    }
    addItem(item, rootItem);
    d->process();
}

void ShareQueueModel::cancel(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    ShareQueueItem *item = static_cast<ShareQueueItem*>(rootItem->children.at(row));
    if (!item->aborted) {
        item->aborted = true;
        foreach (QXmppShareTransfer *transfer, item->transfers.values())
            transfer->abort();
    }
}

bool ShareQueueModel::contains(const QXmppShareItem &shareItem) const
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        ShareQueueItem *item  = static_cast<ShareQueueItem*>(ptr);
        if (item->shareItem.locations() == shareItem.locations())
            return true;
    }
    return false;
}

QVariant ShareQueueModel::data(const QModelIndex &index, int role) const
{
    ShareQueueItem *item = static_cast<ShareQueueItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    const QXmppShareItem *shareItem = &item->shareItem;
    if (role == IsDirRole)
        return shareItem->type() == QXmppShareItem::CollectionItem;
    else if (role == JidRole) {
        if (shareItem->locations().isEmpty())
            return QString();
        else
            return shareItem->locations().first().jid();
    }
    else if (role == NameRole)
        return shareItem->name();
    else if (role == NodeRole) {
        if (shareItem->locations().isEmpty())
            return QString();
        else
            return shareItem->locations().first().node();
    }
    else if (role == SpeedRole) {
        qint64 speed = 0;
        foreach (QXmppShareTransfer *transfer, item->transfers.values())
            speed += transfer->speed();
        return speed;
    }
    else if (role == DoneBytesRole) {
        qint64 done = item->doneBytes;
        foreach (QXmppShareTransfer *transfer, item->transfers.values())
            done += transfer->doneBytes();
        return done;
    }
    else if (role == DoneFilesRole)
        return item->done.size();
    else if (role == TotalBytesRole)
        return item->totalBytes;
    else if (role == TotalFilesRole)
        return item->totalFiles;
    return QVariant();
}

void ShareQueueModel::setManager(QXmppShareManager *manager)
{
    if (manager != d->manager) {
        d->manager = manager;
        if (d->manager) {
            bool check;
            Q_UNUSED(check);

            check = connect(d->manager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                            this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
            Q_ASSERT(check);
        }
    }
}

/** Periodically refresh transfer progress.
 */
void ShareQueueModel::_q_refresh()
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        ShareQueueItem *item = static_cast<ShareQueueItem*>(ptr);
        if (item->transfers.size())
            emit dataChanged(createIndex(item), createIndex(item));
    }
}

void ShareQueueModel::_q_searchReceived(const QXmppShareSearchIq &shareIq)
{
    // filter requests
    if (shareIq.type() == QXmppIq::Get || shareIq.type() == QXmppIq::Set)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        ShareQueueItem *item = static_cast<ShareQueueItem*>(ptr);
        if (item->requestId == shareIq.id()) {
            if (shareIq.type() == QXmppIq::Result) {
                const QString oldName = item->shareItem.name();
                QXmppShareItem *collection = (QXmppShareItem*)&shareIq.collection();
                copy(&item->shareItem, collection);
                item->shareItem.setName(oldName);
                foreach (QXmppShareItem *child, collection->children())
                    item->shareItem.appendChild(*child);
                totals(&item->shareItem, item->totalBytes, item->totalFiles);
                emit dataChanged(createIndex(item), createIndex(item));

                d->process();
            } else {
                removeItem(item);
            }
            return;
        }
    }
}

void ShareQueueModel::_q_transferFinished()
{
    QXmppShareTransfer *transfer = qobject_cast<QXmppShareTransfer*>(sender());
    if (!transfer)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        ShareQueueItem *item = static_cast<ShareQueueItem*>(ptr);
        QXmppShareItem *file = item->transfers.key(transfer);
        if (file) {
            item->done.insert(file);
            item->doneBytes += file->fileSize();
            item->transfers.remove(file);
            transfer->deleteLater();

            // stop periodic refresh
            if (!d->transfers())
                d->timer->stop();

            // check for completion
            if (item->transfers.isEmpty() && !item->nextFile())
                removeItem(item);
            else
                emit dataChanged(createIndex(item), createIndex(item));
            break;
        }
    }

    d->process();
}

SharePlaceModel::SharePlaceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QList<QDesktopServices::StandardLocation> locations;
    locations << QDesktopServices::DocumentsLocation;
    locations << QDesktopServices::MusicLocation;
    locations << QDesktopServices::MoviesLocation;
    locations << QDesktopServices::PicturesLocation;

    foreach (QDesktopServices::StandardLocation location, locations) {
        const QString path = QDesktopServices::storageLocation(location);
        QDir dir(path);
        if (path.isEmpty() || dir == QDir::home())
            continue;

        // do not use "path" directly, on Windows it uses back slashes
        // where the rest of Qt uses forward slashes
        m_paths << dir.path();
    }

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ChatModel::AvatarRole, "avatar");
    roleNames.insert(ChatModel::NameRole, "name");
    roleNames.insert(ChatModel::JidRole, "url");
    setRoleNames(roleNames);
}

QVariant SharePlaceModel::data(const QModelIndex &index, int role) const
{
    const int i = index.row();
    if (!index.isValid())
        return QVariant();

    if (role == ChatModel::AvatarRole)
        return wApp->qmlUrl("128x128/album.png");
    else if (role == ChatModel::NameRole)
        return QFileInfo(m_paths.value(i)).fileName();
    else if (role == ChatModel::JidRole)
        return QUrl::fromLocalFile(m_paths.value(i));

    return QVariant();
}

int SharePlaceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else
        return m_paths.size();
}

ShareFileSystemGet::ShareFileSystemGet(ShareFileSystem *fs, const QXmppShareLocation &location)
    : FileSystemJob(FileSystemJob::Get, fs)
    , m_job(0)
{
    bool check;
    Q_UNUSED(check);

    if (location.jid().isEmpty() || location.node().isEmpty()) {
        setErrorString("Invalid location requested");
        finishLater(FileSystemJob::UrlError);
        return;
    }

    // request file
    foreach (ChatClient *client, ChatClient::instances()) {
        QXmppShareManager *shareManager = client->findExtension<QXmppShareManager>();
        QXmppTransferManager *transferManager = client->findExtension<QXmppTransferManager>();
        if (!shareManager || !transferManager)
            continue;

        check = connect(shareManager, SIGNAL(shareGetIqReceived(QXmppShareGetIq)),
                        this, SLOT(_q_shareGetIqReceived(QXmppShareGetIq)));
        Q_ASSERT(check);

        check = connect(transferManager, SIGNAL(fileReceived(QXmppTransferJob*)),
                        this, SLOT(_q_transferReceived(QXmppTransferJob*)));
        Q_ASSERT(check);

        QXmppShareGetIq iq;
        iq.setTo(location.jid());
        iq.setType(QXmppIq::Get);
        iq.setNode(location.node());
        if (client->sendPacket(iq)) {
            m_packetId = iq.id();

#if 0
            // allow hashing speeds as low as 10MB/s
            const int maxSeconds = qMax(60, int(item.fileSize() / 10000000));
            QTimer::singleShot(maxSeconds * 1000, this, SLOT(_q_timeout()));
#endif
            return;
        }
    }

    // failed to request file
    finishLater(FileSystemJob::UnknownError);
}

void ShareFileSystemGet::_q_shareGetIqReceived(const QXmppShareGetIq &iq)
{
    if (iq.id() != m_packetId)
        return;

    if (iq.type() == QXmppIq::Result) {
        m_sid = iq.sid();
    } else if (iq.type() == QXmppIq::Error) {
        setError(UnknownError);
        setErrorString("Error for file request " + m_packetId);
        emit finished();
    }
}

void ShareFileSystemGet::_q_transferFinished()
{
    if (m_job->error() == QXmppTransferJob::NoError) {
        QFile *fp = new QFile("/tmp/shares");
        fp->open(QIODevice::ReadOnly);
        setData(fp);
    } else {
        setErrorString("Transfer failed");
        setError(UnknownError);
    }
    m_job->deleteLater();
    emit finished();
}

void ShareFileSystemGet::_q_transferReceived(QXmppTransferJob *job)
{
    bool check;
    Q_UNUSED(check);

    if (m_job || job->direction() != QXmppTransferJob::IncomingDirection || job->sid() != m_sid)
        return;

    job->accept("/tmp/shares");
    m_job = job;

    check = connect(m_job, SIGNAL(progress(qint64,qint64)),
                    this, SIGNAL(downloadProgress(qint64,qint64)));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(finished()),
                    this, SLOT(_q_transferFinished()));
    Q_ASSERT(check);
}

class ShareFileSystemPrivate
{
public:
    ShareFileSystemPrivate(ShareFileSystem *qq);
    QList<FileSystemJob*> jobs;
    QSet<QXmppShareManager*> managers;

private:
    ShareFileSystem *q;
};

ShareFileSystemPrivate::ShareFileSystemPrivate(ShareFileSystem *qq)
    : q(qq)
{
}

ShareFileSystem::ShareFileSystem(QObject *parent)
    : FileSystem(parent)
{
    d = new ShareFileSystemPrivate(this);
}

ShareFileSystem::~ShareFileSystem()
{
    delete d;
}

FileSystemJob* ShareFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(type);

    return new ShareFileSystemGet(this, urlToLocation(fileUrl));
}

FileSystemJob* ShareFileSystem::list(const QUrl &dirUrl)
{
    bool check;
    Q_UNUSED(check);
    Q_UNUSED(dirUrl);

    QXmppShareLocation dirLocation = urlToLocation(dirUrl);
    //qDebug("list for jid %s", qPrintable(dirLocation.jid()));
    //qDebug("list for node %s", qPrintable(dirLocation.node()));

    FileSystemJob *job = new FileSystemJob(FileSystemJob::List, this);
    foreach (ChatClient *shareClient, ChatClient::instances()) {
        QXmppShareManager *shareManager = shareClient->findExtension<QXmppShareManager>();
        if (shareManager) {
            if (!d->managers.contains(shareManager)) {
                check = connect(shareManager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                                this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
                Q_ASSERT(check);
                d->managers.insert(shareManager);
            }

            // FIXME: hack
            if (dirLocation.jid().isEmpty()) {
                dirLocation.setJid(shareClient->shareServer());
                dirLocation.setNode(QString());
            }

            const QString requestId = shareManager->search(dirLocation, 1, "");
            job->setProperty("_request_id", requestId);
            job->setProperty("_request_jid", dirLocation.jid());
            job->setProperty("_request_node", dirLocation.node());
            d->jobs.append(job);
            return job;
        }
    }
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

void ShareFileSystem::_q_searchReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get || shareIq.type() == QXmppIq::Set)
        return;

    foreach (FileSystemJob *job, d->jobs) {
        const QString requestId = job->property("_request_id").toString();
        const QString requestJid = job->property("_request_jid").toString();
        if (requestId == shareIq.id() && requestJid == shareIq.from()) {
            if (shareIq.type() == QXmppIq::Result) {
                FileInfoList listItems;
                QXmppShareItem *newItem = (QXmppShareItem*)&shareIq.collection();
                for (int newRow = 0; newRow < newItem->size(); newRow++) {
                    QXmppShareItem *newChild = newItem->child(newRow);
                    if (newChild->locations().isEmpty())
                        return;
                    const QXmppShareLocation location = newChild->locations().first();

                    FileInfo info;
                    info.setName(newChild->name());
                    info.setSize(newChild->fileSize());
                    info.setDir(newChild->type() == QXmppShareItem::CollectionItem);
                    info.setUrl(locationToUrl(newChild->locations().first()));
                    listItems << info;
                }
                job->setError(FileSystemJob::NoError);
                job->setResults(listItems);
            } else {
                job->setError(FileSystemJob::UnknownError);
            }
            d->jobs.removeAll(job);
            QMetaObject::invokeMethod(job, "finished");
            break;
        }
    }
}

class ShareFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("share"))
            return new ShareFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(share_filesystem, ShareFileSystemPlugin)
