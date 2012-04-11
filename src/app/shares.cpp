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

#include <QBuffer>
#include <QCoreApplication>
#include <QDesktopServices>
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

class ShareModelPrivate
{
public:
    ShareModelPrivate(ShareModel *qq);
    void setShareClient(ChatClient *shareClient);

    ChatClient *client;
    bool connected;
    ChatClient *shareClient;
    QString shareServer;

private:
    ShareModel *q;
};

ShareModelPrivate::ShareModelPrivate(ShareModel *qq)
    : client(0),
      connected(false),
      shareClient(0),
      q(qq)
{
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

ShareModel::ShareModel(QObject *parent)
    : QObject(parent)
{
    d = new ShareModelPrivate(this);
}

ShareModel::~ShareModel()
{
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

bool ShareModel::isConnected() const
{
    return d->connected;
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
}

void ShareModel::_q_presenceReceived(const QXmppPresence &presence)
{
    bool check;
    Q_UNUSED(check);

    ChatClient *shareClient = qobject_cast<ChatClient*>(sender());
    if (!shareClient || d->shareServer.isEmpty() || presence.from() != d->shareServer)
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
        QXmppTransferManager *transferManager = shareClient->findExtension<QXmppTransferManager>();
        if (forceProxy == QLatin1String("1") && !transferManager->proxyOnly()) {
            qDebug("Shares forcing SOCKS5 proxy");
            transferManager->setProxyOnly(true);
        }

        // add share manager
        QXmppShareManager *shareManager = shareClient->findExtension<QXmppShareManager>();
        if (!shareManager) {
            shareManager = new QXmppShareManager(shareClient, database());
            shareClient->addExtension(shareManager);
        }

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
        if (shareClient != d->client ||
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

void ShareModel::_q_settingsChanged() const
{
    QStringList dirs;
    foreach (const QVariant &dir, wApp->settings()->sharesDirectories())
        dirs << dir.toUrl().toLocalFile();

    globalDatabase->setDirectory(wApp->settings()->sharesLocation());
    globalDatabase->setMappedDirectories(dirs);
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

ShareFileSystem::ShareFileSystem(QObject *parent)
    : FileSystem(parent)
{
}

FileSystemJob* ShareFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(type);

    return new ShareFileSystemGet(this, urlToLocation(fileUrl));
}

FileSystemJob* ShareFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    return new ShareFileSystemList(this, urlToLocation(dirUrl), filter);
}

class ShareFileSystemBuffer : public QIODevice
{
public:
    ShareFileSystemBuffer(ShareFileSystemGet *parent)
        : QIODevice(parent)
        , m_job(parent)
    {
        setOpenMode(QIODevice::WriteOnly);
    }

protected:
    qint64 readData(char*, qint64)
    {
        return -1;
    }

    qint64 writeData(const char* data, qint64 size)
    {
        return m_job->_q_dataReceived(data, size);
    }

private:
    ShareFileSystemGet *m_job;
};

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

void ShareFileSystemGet::abort()
{
    if (m_job) {
        m_job->abort();
    } else {
        setError(UnknownError);
        emit finished();
    }
}

qint64 ShareFileSystemGet::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + m_buffer.size();
}

bool ShareFileSystemGet::isSequential() const
{
    return true;
}

qint64 ShareFileSystemGet::readData(char *data, qint64 maxSize)
{
    qint64 bytes = qMin(maxSize, qint64(m_buffer.size()));
    memcpy(data, m_buffer.constData(), bytes);
    m_buffer.remove(0, bytes);
    return bytes;
}

qint64 ShareFileSystemGet::_q_dataReceived(const char *data, qint64 size)
{
    m_buffer.append(data, size);
    emit readyRead();
    return size;
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
    if (m_job->error() != QXmppTransferJob::NoError) {
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

    m_job = job;
    m_job->accept(new ShareFileSystemBuffer(this));
    setOpenMode(QIODevice::ReadOnly);

    check = connect(m_job, SIGNAL(progress(qint64,qint64)),
                    this, SIGNAL(downloadProgress(qint64,qint64)));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(finished()),
                    this, SLOT(_q_transferFinished()));
    Q_ASSERT(check);
}

ShareFileSystemList::ShareFileSystemList(ShareFileSystem* fs, const QXmppShareLocation &dirLocation, const QString &filter)
    : FileSystemJob(FileSystemJob::List, fs)
{
    bool check;
    Q_UNUSED(check);

    QXmppShareLocation location(dirLocation);

    foreach (ChatClient *shareClient, ChatClient::instances()) {
        QXmppShareManager *shareManager = shareClient->findExtension<QXmppShareManager>();
        if (!shareManager)
            continue;

        check = connect(shareManager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                        this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
        Q_ASSERT(check);

        // FIXME: hack
        if (location.jid().isEmpty()) {
            location.setJid(shareClient->shareServer());
            location.setNode(QString());
        }

        m_packetId = shareManager->search(location, 1, filter);
        m_jid = location.jid();
        return;
    }
    
    // failed to request list
    finishLater(FileSystemJob::UnknownError);
}

void ShareFileSystemList::_q_searchReceived(const QXmppShareSearchIq &shareIq)
{
    if ((shareIq.type() != QXmppIq::Result && shareIq.type() != QXmppIq::Error)
        || shareIq.from() != m_jid
        || shareIq.id() != m_packetId)
        return;

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
        setError(FileSystemJob::NoError);
        setResults(listItems);
    } else {
        setError(FileSystemJob::UnknownError);
    }
    emit finished();
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
