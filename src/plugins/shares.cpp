/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include <QDesktopServices>
#include <QSettings>
#include <QTimer>

#include "QDjango.h"
#include "QXmppClient.h"
#include "QXmppPresence.h"
#include "QXmppShareExtension.h"
#include "QXmppShareDatabase.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "chat_client.h"
#include "chat_utils.h"
#include "roster.h"
#include "shares.h"

// common queries
#define Q ShareModelQuery
#define Q_FIND_LOCATIONS(locations)  Q(QXmppShareItem::LocationsRole, Q::Equals, QVariant::fromValue(locations))

static QXmppShareDatabase *globalDatabase = 0;
static int globalDatabaseRefs = 0;

static QXmppShareDatabase *refDatabase()
{
    if (!globalDatabase)
    {
        // initialise database
        const QString databaseName = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).filePath("database.sqlite");
        QSqlDatabase sharesDb = QSqlDatabase::addDatabase("QSQLITE");
        sharesDb.setDatabaseName(databaseName);
        Q_ASSERT(sharesDb.open());
        QDjango::setDatabase(sharesDb);
        // drop wiLink <= 0.9.4 table
        sharesDb.exec("DROP TABLE files");

        // sanitize settings
        QSettings settings;
        QString sharesDirectory = settings.value("SharesLocation",  QDir::home().filePath("Public")).toString();
        if (sharesDirectory.endsWith("/"))
            sharesDirectory.chop(1);
        QStringList mappedDirectories = settings.value("SharesDirectories").toStringList();

        // create shares database
        globalDatabase = new QXmppShareDatabase;
        globalDatabase->setDirectory(sharesDirectory);
        globalDatabase->setMappedDirectories(mappedDirectories);
    }
    globalDatabaseRefs++;
    return globalDatabase;
}

static void unrefDatabase()
{
    if (!(--globalDatabaseRefs) && globalDatabase) {
        delete globalDatabase;
        globalDatabase = 0; 
    }
}

/** Update collection timestamps.
 */
static void updateTime(QXmppShareItem *oldItem, const QDateTime &stamp)
{
    if (oldItem->type() == QXmppShareItem::CollectionItem && oldItem->size() > 0)
    {
        oldItem->setData(UpdateTime, QDateTime::currentDateTime());
        for (int i = 0; i < oldItem->size(); i++)
            updateTime(oldItem->child(i), stamp);
    }
}

class ShareModelPrivate
{
public:
    ShareModelPrivate(ShareModel *qq);
    void setShareClient(ChatClient *shareClient);
    QXmppShareExtension *shareManager();

    ChatClient *client;
    QString filter;
    QString rootJid;
    QString rootNode;
    ChatClient *shareClient;
    QString shareServer;
    QTimer *timer;

private:
    ShareModel *q;
};

ShareModelPrivate::ShareModelPrivate(ShareModel *qq)
    : client(0),
      shareClient(0),
      q(qq)
{
}

void ShareModelPrivate::setShareClient(ChatClient *newClient)
{
    if (newClient == shareClient)
        return;

    // delete old share client
    if (shareClient && shareClient != client)
        shareClient->deleteLater();

    // set new share client
    shareClient = newClient;
    if (shareClient) {
        bool check;

        check = q->connect(shareClient, SIGNAL(disconnected()),
                           q, SLOT(_q_disconnected()));
        Q_ASSERT(check);

        check = q->connect(shareClient, SIGNAL(presenceReceived(const QXmppPresence&)),
                           q, SLOT(_q_presenceReceived(const QXmppPresence&)));
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

QXmppShareExtension *ShareModelPrivate::shareManager()
{
    if (shareClient)
        return shareClient->findExtension<QXmppShareExtension>();
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
    roleNames.insert(IsDirRole, "isDir");
    roleNames.insert(JidRole, "jid");
    roleNames.insert(NameRole, "name");
    roleNames.insert(NodeRole, "node");
    roleNames.insert(SizeRole, "size");
    setRoleNames(roleNames);

    // add timer
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    d->timer->setInterval(100);
    connect(d->timer, SIGNAL(timeout()), this, SLOT(refresh()));
}

ShareModel::~ShareModel()
{
    delete rootItem;
    delete d;
}

QXmppShareItem *ShareModel::addItem(const QXmppShareItem &item)
{
   beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
   QXmppShareItem *child = rootItem->appendChild(item);
   endInsertRows();
   return child;
}

ChatClient *ShareModel::client() const
{
    return d->client;
}

void ShareModel::setClient(ChatClient *client)
{
    if (client != d->client) {
        d->client = client;
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

    if (role == IsDirRole)
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
    else if (role == SizeRole)
        return item->fileSize();
    else if (role == ProgressRole) {
        const QString localPath = item->data(TransferPath).toString();
        qint64 done = item->data(TransferDone).toLongLong();
        if (!localPath.isEmpty())
            return tr("Downloaded");
        else if (item->data(TransferError).toInt())
            return tr("Failed");
        else if (done > 0)
        {
            qint64 total = index.data(TransferTotal).toLongLong();
            int progress = total ? ((done * 100) / total) : 0;
            return QString::number(progress) + "%";
        }
        else if (!item->data(PacketId).toString().isEmpty())
            return tr("Requested");
        else
            return tr("Queued");
    }
    return item->data(role);
}

QList<QXmppShareItem *> ShareModel::filter(const ShareModelQuery &query, const ShareModel::QueryOptions &options, QXmppShareItem *parent, int limit)
{
    if (!parent)
        parent = rootItem;

    QList<QXmppShareItem*> matches;
    QXmppShareItem *child;

    // pre-recurse
    if (options.recurse == PreRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }

    // look at immediate children
    for (int i = 0; i < parent->size(); i++)
    {
        child = parent->child(i);
        if (query.match(child))
            matches.append(child);
        if (limit && matches.size() == limit)
            return matches;
    }

    // post-recurse
    if (options.recurse == PostRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }
    return matches;
}

QXmppShareItem *ShareModel::get(const ShareModelQuery &query, const ShareModel::QueryOptions &options, QXmppShareItem *parent)
{
    QList<QXmppShareItem*> match = filter(query, options, parent, 1);
    if (match.size())
        return match.first();
    return 0;
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
    QXmppShareExtension *shareManager = d->shareManager();
    if (!shareManager || d->rootJid.isEmpty())
        return;

    // browse files
    shareManager->search(QXmppShareLocation(d->rootJid, d->rootNode), 1, d->filter);
}

void ShareModel::removeItem(QXmppShareItem *item)
{
    if (!item || item == rootItem)
        return;

    QXmppShareItem *parentItem = item->parent();
    beginRemoveRows(createIndex(parentItem), item->row(), item->row());
    parentItem->removeChild(item);
    endRemoveRows();
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

QModelIndex ShareModel::updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem)
{
    QDateTime stamp = QDateTime::currentDateTime();

    QModelIndex oldIndex;
    if (!oldItem)
        oldItem = rootItem;
    if (oldItem != rootItem)
        oldIndex = createIndex(oldItem);

    // update own data
    oldItem->setFileHash(newItem->fileHash());
    oldItem->setFileSize(newItem->fileSize());
    oldItem->setLocations(newItem->locations());
    // root collections have empty names
    if (!newItem->name().isEmpty())
        oldItem->setName(newItem->name());
    oldItem->setType(newItem->type());
    if (oldItem != rootItem)
        emit dataChanged(oldIndex, oldIndex);

    // if we received an empty collection, stop here
    if (newItem->type() == QXmppShareItem::CollectionItem && !newItem->size())
        return oldIndex;

    // if we received a file, clear any children and stop here
    if (newItem->type() == QXmppShareItem::FileItem)
    {
        if (oldItem->size())
        {
            beginRemoveRows(oldIndex, 0, oldItem->size());
            oldItem->clearChildren();
            endRemoveRows();
        }
        return oldIndex;
    }

    // update own timestamp
    oldItem->setData(UpdateTime, stamp);

    // update existing children
    QList<QXmppShareItem*> removed = oldItem->children();
    for (int newRow = 0; newRow < newItem->size(); newRow++)
    {
        QXmppShareItem *newChild = newItem->child(newRow);
        QXmppShareItem *oldChild = get(Q_FIND_LOCATIONS(newChild->locations()), QueryOptions(DontRecurse), oldItem);
        if (oldChild)
        {
            // update existing child
            const int oldRow = oldChild->row();
            if (oldRow != newRow)
            {
                beginMoveRows(oldIndex, oldRow, oldRow, oldIndex, newRow);
                oldItem->moveChild(oldRow, newRow);
                endMoveRows();
            }
            updateItem(oldChild, newChild);
            removed.removeAll(oldChild);
        } else {
            // insert new child
            beginInsertRows(oldIndex, newRow, newRow);
            QXmppShareItem *item = oldItem->insertChild(newRow, *newChild);
            updateTime(item, stamp);
            endInsertRows();
        }
    }

    // remove old children
    foreach (QXmppShareItem *oldChild, removed)
    {
        beginRemoveRows(oldIndex, oldChild->row(), oldChild->row());
        oldItem->removeChild(oldChild);
        endRemoveRows();
    }

    return oldIndex;
}

void ShareModel::_q_disconnected()
{

}

void ShareModel::_q_presenceReceived(const QXmppPresence &presence)
{
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
        if (forceProxy == "1" && !transferManager->proxyOnly())
        {
            qDebug("Forcing SOCKS5 proxy");
            transferManager->setProxyOnly(true);
        }

        // add share manager
        QXmppShareExtension *shareManager = d->shareClient->findExtension<QXmppShareExtension>();
        if (!shareManager) {
            bool check;

            shareManager = new QXmppShareExtension(d->shareClient, refDatabase());
            d->shareClient->addExtension(shareManager);

            check = connect(shareManager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                            this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
            Q_ASSERT(check);

#if 0
            check = connect(extension, SIGNAL(getFailed(QString)),
                            this, SLOT(getFailed(QString)));
            Q_ASSERT(check);

            check = connect(extension, SIGNAL(transferFinished(QXmppTransferJob*)),
                            this, SLOT(transferFinished(QXmppTransferJob*)));
            Q_ASSERT(check);

            check = connect(extension, SIGNAL(transferStarted(QXmppTransferJob*)),
                            this, SLOT(transferStarted(QXmppTransferJob*)));
            Q_ASSERT(check);
#endif
        }

        emit shareServerChanged(d->shareServer);
    }
    else if (presence.type() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString domain = shareExtension.firstChildElement("domain").value();
        const QString server = shareExtension.firstChildElement("server").value();

        qDebug("Redirecting to %s,%s", qPrintable(domain), qPrintable(server));

        // reconnect to another server
        QXmppConfiguration config = d->client->configuration();
        config.setDomain(domain);
        config.setHost(server);

        ChatClient *newClient = new ChatClient(this);
        newClient->setLogger(d->client->logger());

        // replace client
        d->setShareClient(newClient);
        newClient->connectToServer(config);
    }
}

void ShareModel::_q_serverChanged(const QString &server)
{
    Q_ASSERT(d->shareClient);

    d->shareServer = server;
    if (d->shareServer.isEmpty())
        return;

    qDebug("registering with %s", qPrintable(d->shareServer));

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
    qDebug("got shareIq from %s", qPrintable(shareIq.from()));
    if (shareIq.type() == QXmppIq::Get || shareIq.from() != d->rootJid)
        return;

    if (shareIq.type() == QXmppIq::Error)
    {
        removeRows(0, rootItem->size());
    } else {
        updateItem(rootItem, (QXmppShareItem*)&shareIq.collection());
    }
}

ShareModelQuery::ShareModelQuery()
    :  m_role(0), m_operation(None), m_combine(NoCombine)
{
}

ShareModelQuery::ShareModelQuery(int role, ShareModelQuery::Operation operation, QVariant data)
    :  m_role(role), m_operation(operation), m_data(data), m_combine(NoCombine)
{
}

bool ShareModelQuery::match(QXmppShareItem *item) const
{
    if (m_operation == Equals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return item->locations() == m_data.value<QXmppShareLocationList>();
        return item->data(m_role) == m_data;
    }
    else if (m_operation == NotEquals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return !(item->locations() == m_data.value<QXmppShareLocationList>());
        return (item->data(m_role) != m_data);
    }
    else if (m_combine == AndCombine)
    {
        foreach (const ShareModelQuery &child, m_children)
            if (!child.match(item))
                return false;
        return true;
    }
    else if (m_combine == OrCombine)
    {
        foreach (const ShareModelQuery &child, m_children)
            if (child.match(item))
                return true;
        return false;
    }
    // empty query matches all
    return true;
}

ShareModelQuery ShareModelQuery::operator&&(const ShareModelQuery &other) const
{
    ShareModelQuery result;
    result.m_combine = AndCombine;
    result.m_children << *this << other;
    return result;
}

ShareModelQuery ShareModelQuery::operator||(const ShareModelQuery &other) const
{
    ShareModelQuery result;
    result.m_combine = OrCombine;
    result.m_children << *this << other;
    return result;
}

ShareModel::QueryOptions::QueryOptions(Recurse recurse_)
    : recurse(recurse_)
{
}


