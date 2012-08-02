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

#include <QDesktopServices>
#include <QTimer>

#include "QDjango.h"
#include "QXmppClient.h"
#include "QXmppPresence.h"
#include "QXmppShareDatabase.h"
#include "QXmppShareManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "client.h"
#include "roster.h"
#include "settings.h"
#include "shares.h"

Q_GLOBAL_STATIC(ShareWatcher, theShareWatcher);

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

ShareWatcher::ShareWatcher(QObject *parent)
    : QObject(parent)
    , m_shareDatabase(0)
{
    qDebug("ShareWatcher created");

    // monitor clients
    foreach (ChatClient *client, ChatClient::instances())
        _q_clientCreated(client);
    connect(ChatClient::observer(), SIGNAL(clientCreated(ChatClient*)),
            this, SLOT(_q_clientCreated(ChatClient*)));
    connect(ChatClient::observer(), SIGNAL(clientDestroyed(ChatClient*)),
            this, SLOT(_q_clientDestroyed(ChatClient*)));
}

ShareWatcher::~ShareWatcher()
{
    qDebug("ShareWatcher destroyed");
}

void ShareWatcher::_q_clientCreated(ChatClient *client)
{
    bool check;
    Q_UNUSED(check);

    check = connect(client, SIGNAL(disconnected()),
                    this, SLOT(_q_disconnected()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(_q_presenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(shareServerChanged(QString)),
                    this, SLOT(_q_serverChanged(QString)));
    Q_ASSERT(check);

    // if we already know the server, register with it
    const QString server = client->shareServer();
    if (!server.isEmpty()) {
        QMetaObject::invokeMethod(client, "shareServerChanged", Q_ARG(QString, server));
    }
}

void ShareWatcher::_q_clientDestroyed(ChatClient *client)
{
    if (m_shareDatabase && ChatClient::instances().isEmpty()) {
        delete m_shareDatabase;
        m_shareDatabase = 0;
    }
}

QXmppShareDatabase *ShareWatcher::database()
{
    bool check;
    Q_UNUSED(check);

    if (!m_shareDatabase) {
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
        m_shareDatabase = new QXmppShareDatabase(this);
        check = connect(wSettings, SIGNAL(sharesDirectoriesChanged(QVariantList)),
                        this, SLOT(_q_settingsChanged()));
        Q_ASSERT(check);
        check = connect(wSettings, SIGNAL(sharesLocationChanged(QString)),
                        this, SLOT(_q_settingsChanged()));
        Q_ASSERT(check);
        _q_settingsChanged();
    }

    return m_shareDatabase;
}

void ShareWatcher::_q_disconnected()
{
    emit isConnectedChanged();
}

void ShareWatcher::_q_presenceReceived(const QXmppPresence &presence)
{
    bool check;
    Q_UNUSED(check);

    ChatClient *client = qobject_cast<ChatClient*>(sender());
    if (!client || client->shareServer().isEmpty() || presence.from() != client->shareServer())
        return;

    // find shares extension
    QXmppElement shareExtension;
    foreach (const QXmppElement &extension, presence.extensions()) {
        if (extension.attribute("xmlns") == ns_shares) {
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
        QXmppTransferManager *transferManager = client->findExtension<QXmppTransferManager>();
        if (forceProxy == QLatin1String("1") && !transferManager->proxyOnly()) {
            qDebug("Shares forcing SOCKS5 proxy");
            transferManager->setProxyOnly(true);
        }

        // add share manager
        QXmppShareManager *shareManager = client->findExtension<QXmppShareManager>();
        if (!shareManager) {
            shareManager = new QXmppShareManager(client, database());
            client->addExtension(shareManager);

            check = connect(shareManager, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                            this, SLOT(_q_searchReceived(QXmppShareSearchIq)));
            Q_ASSERT(check);
        }

        emit isConnectedChanged();
    }
    else if (presence.type() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString newDomain = shareExtension.firstChildElement("domain").value();

        // avoid redirect loop
        foreach (ChatClient *client, ChatClient::instances()) {
            if (client->configuration().domain() == newDomain) {
                qWarning("Shares not redirecting to domain %s", qPrintable(newDomain));
                return;
            }
        }

        // reconnect to another server
        qDebug("Shares redirecting to %s", qPrintable(newDomain));
        ChatClient *newClient = new ChatClient(client);
        newClient->setProperty("_parent_jid", client->jid());
        newClient->setLogger(client->logger());

        const QString newJid = client->configuration().user() + '@' + newDomain;
        newClient->connectToServer(newJid, client->configuration().password());
    }
}

void ShareWatcher::_q_searchReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Set)
        emit isConnectedChanged();
}

void ShareWatcher::_q_settingsChanged() const
{
    QStringList dirs;
    foreach (const QVariant &dir, wSettings->sharesDirectories())
        dirs << dir.toUrl().toLocalFile();

    m_shareDatabase->setDirectory(wSettings->sharesLocation());
    m_shareDatabase->setMappedDirectories(dirs);
}

void ShareWatcher::_q_serverChanged(const QString &server)
{
    ChatClient *client = qobject_cast<ChatClient*>(sender());
    if (!client)
        return;

    qDebug("Shares registering with %s", qPrintable(server));

    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    // get nickname
    QString nickJid = client->property("_parent_jid").toString();
    if (nickJid.isEmpty())
        nickJid = client->jid();
    VCard card;
    card.setJid(QXmppUtils::jidToBareJid(nickJid));

    QXmppElement nickName;
    nickName.setTagName("nickName");
    nickName.setValue(card.name());
    x.appendChild(nickName);

    QXmppPresence presence;
    presence.setTo(server);
    presence.setExtensions(QXmppElementList() << x);
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNone);
    client->sendPacket(presence);
}

ShareFileSystem::ShareFileSystem(QObject *parent)
    : FileSystem(parent)
{
    bool check;
    Q_UNUSED(check);

    check = connect(theShareWatcher(), SIGNAL(isConnectedChanged()),
                    this, SLOT(_q_changed()));
    Q_ASSERT(check);
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

void ShareFileSystem::_q_changed()
{
    emit directoryChanged(QUrl("share:"));
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

