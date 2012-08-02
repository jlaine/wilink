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

#ifndef __WILINK_SHARES_MODEL_H__
#define __WILINK_SHARES_MODEL_H__

#include "QXmppShareIq.h"

#include "filesystem.h"
#include "model.h"

using namespace QNetIO;

class ChatClient;
class QXmppPresence;
class QXmppShareDatabase;
class QXmppShareManager;
class QXmppTransferJob;

class ShareWatcher : public QObject
{
    Q_OBJECT

public:
    ShareWatcher(QObject *parent = 0);

signals:
    void isConnectedChanged();

private slots:
    void _q_clientCreated(ChatClient *client);
    void _q_disconnected();
    void _q_presenceReceived(const QXmppPresence &presence);
    void _q_searchReceived(const QXmppShareSearchIq &shareIq);
    void _q_serverChanged(const QString &server);
    void _q_settingsChanged() const;

private:
    QXmppShareDatabase *database();

    QXmppShareDatabase *m_shareDatabase;
};

class ShareFileSystem : public FileSystem
{
    Q_OBJECT

public:
    ShareFileSystem(QObject *parent = 0);
    FileSystemJob* get(const QUrl &fileUrl, ImageSize type);
    FileSystemJob* list(const QUrl &dirUrl, const QString &filter = QString());

private slots:
    void _q_changed();
};

class ShareFileSystemGet : public FileSystemJob
{
    Q_OBJECT

public:
    ShareFileSystemGet(ShareFileSystem *fs, const QXmppShareLocation &location);

    qint64 bytesAvailable() const;
    bool isSequential() const;
    qint64 _q_dataReceived(const char* data, qint64 bytes);

public slots:
    void abort();

private slots:
    void _q_shareGetIqReceived(const QXmppShareGetIq &iq);
    void _q_transferFinished();
    void _q_transferReceived(QXmppTransferJob *job);

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    QByteArray m_buffer;
    QXmppTransferJob* m_job;
    QString m_packetId;
    QString m_sid;
};

class ShareFileSystemList : public FileSystemJob
{
    Q_OBJECT

public:
    ShareFileSystemList(ShareFileSystem *fs, const QXmppShareLocation &location, const QString &filter);

private slots:
    void _q_searchReceived(const QXmppShareSearchIq &shareIq);

private:
    QString m_jid;
    QString m_packetId;
};

#endif
