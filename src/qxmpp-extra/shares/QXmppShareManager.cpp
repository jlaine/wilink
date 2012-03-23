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

#include <QDateTime>
#include <QDomElement>
#include <QTimer>

#include "QXmppClient.h"
#include "QXmppShareDatabase.h"
#include "QXmppShareIq.h"
#include "QXmppShareManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

static QString availableFilePath(const QString &dirPath, const QString &name)
{
    QString fileName = name;
    QDir downloadsDir(dirPath);
    if (downloadsDir.exists(fileName))
    {
        const QString fileBase = QFileInfo(fileName).completeBaseName();
        const QString fileSuffix = QFileInfo(fileName).suffix();
        int i = 2;
        while (downloadsDir.exists(fileName))
        {
            fileName = QString("%1_%2").arg(fileBase, QString::number(i++));
            if (!fileSuffix.isEmpty())
                fileName += "." + fileSuffix;
        }
    }
    return downloadsDir.absoluteFilePath(fileName);
}

QXmppShareTransfer::QXmppShareTransfer(QXmppClient *client, const QXmppShareItem &item, QObject *parent)
    : QXmppLoggable(parent),
      m_client(client),
      m_error(false),
      m_job(0),
      m_state(RequestState),
      m_doneBytes(0),
      m_totalBytes(0)
{
    bool check;
    Q_UNUSED(check);

    // determine full path
    QStringList pathBits;
    QXmppShareItem *parentItem = ((QXmppShareItem*)&item)->parent();
    while (parentItem && !parentItem->name().isEmpty()) {
        // sanitize path
        QString dirName = parentItem->name();
        if (dirName != "." && dirName != ".." && !dirName.contains("/") && !dirName.contains("\\"))
            pathBits.prepend(dirName);
        parentItem = parentItem->parent();
    }
    m_targetDir = pathBits.join("/");

    // request file
    foreach (const QXmppShareLocation &location, item.locations()) {
        if (!location.jid().isEmpty() &&
            !location.node().isEmpty())
        {
            QXmppShareGetIq iq;
            iq.setTo(location.jid());
            iq.setType(QXmppIq::Get);
            iq.setNode(location.node());
            if (m_client->sendPacket(iq)) {
                QXmppTransferManager *transferManager = m_client->findExtension<QXmppTransferManager>();
                check = connect(transferManager, SIGNAL(fileReceived(QXmppTransferJob*)),
                                this, SLOT(_q_transferReceived(QXmppTransferJob*)));
                Q_ASSERT(check);

                // allow hashing speeds as low as 10MB/s
                const int maxSeconds = qMax(60, int(item.fileSize() / 10000000));

                packetId = iq.id();
                QTimer::singleShot(maxSeconds * 1000, this, SLOT(_q_timeout()));
                return;
            }
        }
    }

    // failed to request file
    m_error = true;
    setState(FinishedState);
}

void QXmppShareTransfer::abort()
{
    if (m_job) {
        m_job->abort();
    } else {
        m_error = true;
        setState(FinishedState);
    }
}

qint64 QXmppShareTransfer::doneBytes() const
{
    return m_doneBytes;
}

qint64 QXmppShareTransfer::totalBytes() const
{
    return m_totalBytes;
}

bool QXmppShareTransfer::error() const
{
    return m_error;
}

qint64 QXmppShareTransfer::speed() const
{
    if (m_job)
        return m_job->speed();
    else
        return 0;
}

QXmppShareTransfer::State QXmppShareTransfer::state() const
{
    return m_state;
}

void QXmppShareTransfer::setState(QXmppShareTransfer::State state)
{
    if (state != m_state) {
        m_state = state;
        emit stateChanged(m_state);
        if (m_state == FinishedState)
            QTimer::singleShot(0, this, SIGNAL(finished()));
    }
}
void QXmppShareTransfer::_q_getFinished(QXmppShareGetIq &iq)
{
    if (m_state != RequestState)
        return;

    if (iq.type() == QXmppIq::Result)
        m_sid = iq.sid();
    else if (iq.type() == QXmppIq::Error) {
        warning("Error for file request " + packetId);
        m_error = true;
        setState(FinishedState);
    }
}

void QXmppShareTransfer::_q_timeout()
{
    if (m_state == RequestState) {
        warning("Timeout for file request " + packetId);
        m_error = true;
        setState(FinishedState);
    }
}

void QXmppShareTransfer::_q_transferFinished()
{
    Q_ASSERT(m_job);
    const QString localPath = m_job->localFileUrl().toLocalFile();

    if (m_job->error() == QXmppTransferJob::NoError) {
        // rename file
        QFileInfo tempInfo(localPath);
        QDir tempDir(tempInfo.dir());
        QString finalPath = availableFilePath(tempDir.path(), tempInfo.fileName().remove(QRegExp("\\.part$")));
        tempDir.rename(tempInfo.fileName(), QFileInfo(finalPath).fileName());

        // store to shares database
        QXmppShareDatabase *db = m_client->findExtension<QXmppShareManager>()->database();
        File cached;
        cached.setPath(db->fileNode(finalPath));
        cached.setSize(m_job->fileSize());
        cached.setHash(m_job->fileHash());
        cached.setDate(QFileInfo(finalPath).lastModified());
        cached.save();
    } else {
        // remove temporary file
        warning("Error for file transfer " + packetId);
        QFile(localPath).remove();
        m_error = true;
    }

    // delete job
    m_job->deleteLater();
    m_job = 0;
    setState(FinishedState);
}

void QXmppShareTransfer::_q_transferProgress(qint64 done, qint64 total)
{
    m_doneBytes = done;
    m_totalBytes = total;
}

void QXmppShareTransfer::_q_transferReceived(QXmppTransferJob *job)
{
    bool check;
    Q_UNUSED(check);

    if (m_job || job->direction() != QXmppTransferJob::IncomingDirection || job->sid() != m_sid)
        return;
    m_job = job;

    // create directory
    QXmppShareDatabase *db = m_client->findExtension<QXmppShareManager>()->database();
    QString dirPath = db->directory();
    if (!m_targetDir.isEmpty()) {
        QDir dir(dirPath);
        if (dir.exists(m_targetDir) || dir.mkpath(m_targetDir))
            dirPath = dir.filePath(m_targetDir);
    }

    // accept transfer
    check = connect(m_job, SIGNAL(finished()),
                    this, SLOT(_q_transferFinished()));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(progress(qint64,qint64)),
                    this, SLOT(_q_transferProgress(qint64,qint64)));
    Q_ASSERT(check);

    m_job->accept(availableFilePath(dirPath, m_job->fileName() + ".part"));

    setState(TransferState);
}

class QXmppShareManagerPrivate
{
public:
    QXmppShareDatabase *database;
    QList<QXmppShareTransfer*> requests;
    QXmppTransferManager *transferManager;
};

QXmppShareManager::QXmppShareManager(QXmppClient *client, QXmppShareDatabase *db)
    : d(new QXmppShareManagerPrivate)
{
    bool check;
    Q_UNUSED(check);

    d->database = db;

    check = connect(db, SIGNAL(getFinished(QXmppShareGetIq,QXmppShareItem)),
                    this, SLOT(getFinished(QXmppShareGetIq,QXmppShareItem)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                    db, SLOT(search(QXmppShareSearchIq)));
    Q_ASSERT(check);

    check = connect(db, SIGNAL(searchFinished(QXmppShareSearchIq)),
                    this, SLOT(searchFinished(QXmppShareSearchIq)));
    Q_ASSERT(check);

    // make sure we have a transfer manager
    d->transferManager = client->findExtension<QXmppTransferManager>();
    if (!d->transferManager) {
        d->transferManager = new QXmppTransferManager;
        client->addExtension(d->transferManager);
    }
}

QXmppShareManager::~QXmppShareManager()
{
    delete d;
}

QXmppShareDatabase *QXmppShareManager::database() const
{
    return d->database;
}

QStringList QXmppShareManager::discoveryFeatures() const
{
    return QStringList() << ns_shares;
}

bool QXmppShareManager::handleStanza(const QDomElement &element)
{
    if (element.tagName() == "iq")
    {
        if (QXmppShareGetIq::isShareGetIq(element))
        {
            QXmppShareGetIq getIq;
            getIq.parse(element);

            if (getIq.type() == QXmppIq::Get)
                d->database->get(getIq);
            else if (getIq.type() == QXmppIq::Result || getIq.type() == QXmppIq::Error) {
                foreach (QXmppShareTransfer *transfer, d->requests) {
                    if (transfer->packetId == getIq.id()) {
                        transfer->_q_getFinished(getIq);
                        break;
                    }
                }
            }
            return true;
        }
        else if (QXmppShareSearchIq::isShareSearchIq(element))
        {
            QXmppShareSearchIq searchIq;
            searchIq.parse(element);
            emit shareSearchIqReceived(searchIq);
            return true;
        }
    }
    return false;
}

QXmppShareTransfer *QXmppShareManager::get(const QXmppShareItem &item)
{
    bool check;
    Q_UNUSED(check);

    if (item.type() != QXmppShareItem::FileItem) {
        warning("Cannot download collection: " + item.name());
        return 0;
    }

    // register request
    QXmppShareTransfer *transfer = new QXmppShareTransfer(client(), item, this);
    if (transfer->state() == QXmppShareTransfer::FinishedState) {
        delete transfer;
        return 0;
    }

    check = connect(transfer, SIGNAL(destroyed(QObject*)),
                    this, SLOT(_q_transferDestroyed(QObject*)));
    Q_ASSERT(check);

    d->requests << transfer;
    return transfer;
}

void QXmppShareManager::getFinished(const QXmppShareGetIq &iq, const QXmppShareItem &shareItem)
{
    bool check;
    Q_UNUSED(check);

    // check the IQ the response is for this connection
    if (iq.from() != client()->configuration().jid())
        return;

    QXmppShareGetIq responseIq(iq);

    // FIXME: for some reason, random number generation in thread is broken
    if (responseIq.type() != QXmppIq::Error)
        responseIq.setSid(generateStanzaHash());
    client()->sendPacket(responseIq);

    // send file
    if (responseIq.type() != QXmppIq::Error)
    {
        QString filePath = d->database->filePath(shareItem.locations()[0].node());
        QXmppTransferFileInfo fileInfo;
        fileInfo.setName(shareItem.name());
        fileInfo.setDate(shareItem.fileDate());
        fileInfo.setHash(shareItem.fileHash());
        fileInfo.setSize(shareItem.fileSize());

        info("Sending file: " + filePath);

        QFile *file = new QFile(filePath);
        file->open(QIODevice::ReadOnly);
        QXmppTransferJob *job = d->transferManager->sendFile(responseIq.to(), file, fileInfo, responseIq.sid());
        job->setLocalFileUrl(QUrl::fromLocalFile(filePath));
        file->setParent(job);

        check = connect(job, SIGNAL(finished()),
                        job, SLOT(deleteLater()));
        Q_ASSERT(check);
    }
}

QString QXmppShareManager::search(const QXmppShareLocation &location, int depth, const QString &query)
{
    QXmppShareSearchIq iq;
    iq.setTo(location.jid());
    iq.setType(QXmppIq::Get);
    iq.setDepth(depth);
    iq.setNode(location.node());
    iq.setSearch(query);

    if (!client()->sendPacket(iq))
        return QString();

    return iq.id();
}

void QXmppShareManager::searchFinished(const QXmppShareSearchIq &responseIq)
{
    // check the IQ the response is for this connection
    if (responseIq.from() != client()->configuration().jid())
        return;

    client()->sendPacket(responseIq);
}

void QXmppShareManager::_q_transferDestroyed(QObject *object)
{
    d->requests.removeAll(static_cast<QXmppShareTransfer*>(object));
}

