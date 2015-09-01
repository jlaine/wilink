/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

#ifndef __WILINK_CHAT_CLIENT_H__
#define __WILINK_CHAT_CLIENT_H__

#include "QXmppClient.h"
#include "QXmppMessage.h"

class ChatClientPrivate;
class DiagnosticManager;
class QDateTime;
class QHostInfo;
class QXmppArchiveManager;
class QXmppCallManager;
class QXmppDiscoveryIq;
class QXmppDiscoveryManager;
class QXmppEntityTimeIq;
class QXmppMucManager;
class QXmppTransferManager;

class ChatClient;

class ChatClientObserver : public QObject
{
    Q_OBJECT

public:
    ChatClientObserver();

signals:
    void clientCreated(ChatClient *client);
    void clientDestroyed(ChatClient *client);

    friend class ChatClient;
};

class ChatClient : public QXmppClient
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid NOTIFY jidChanged)
    Q_PROPERTY(QString statusType READ statusType WRITE setStatusType NOTIFY statusTypeChanged)
    Q_PROPERTY(QXmppArchiveManager* archiveManager READ archiveManager CONSTANT)
    Q_PROPERTY(QXmppCallManager* callManager READ callManager CONSTANT)
    Q_PROPERTY(DiagnosticManager* diagnosticManager READ diagnosticManager CONSTANT)
    Q_PROPERTY(QString diagnosticServer READ diagnosticServer NOTIFY diagnosticServerChanged)
    Q_PROPERTY(QXmppDiscoveryManager* discoveryManager READ discoveryManager CONSTANT)
    Q_PROPERTY(QXmppMucManager* mucManager READ mucManager CONSTANT)
    Q_PROPERTY(QString mucServer READ mucServer NOTIFY mucServerChanged)
    Q_PROPERTY(QXmppRosterManager* rosterManager READ rosterManager CONSTANT)
    Q_PROPERTY(QXmppTransferManager* transferManager READ transferManager CONSTANT)

public:
    ChatClient(QObject *parent = 0);
    ~ChatClient();
    QString jid() const;
    QDateTime serverTime() const;

    QString statusType() const;
    void setStatusType(const QString &statusType);

    QXmppArchiveManager *archiveManager();
    QXmppCallManager *callManager();
    DiagnosticManager *diagnosticManager();
    QString diagnosticServer() const;
    QXmppDiscoveryManager *discoveryManager();
    QXmppMucManager *mucManager();
    QString mucServer() const;
    QXmppRosterManager *rosterManager();
    QXmppTransferManager* transferManager();

    static QList<ChatClient*> instances();
    static ChatClientObserver* observer();

    static QString statusToString(QXmppPresence::AvailableStatusType type);
    static QXmppPresence::AvailableStatusType stringToStatus(const QString& str);

signals:
    void authenticationFailed();
    void conflictReceived();
    void jidChanged(const QString &jid);
    void diagnosticServerChanged(const QString &diagnosticServer);
    void messageReceived(const QString &from);
    void mucServerChanged(const QString &mucServer);
    void statusTypeChanged(const QString &statusType);

public slots:
    void connectToFacebook(const QString &appId, const QString &accessToken);
    void connectToGoogle(const QString &jid, const QString &accessToken);
    void connectToWindowsLive(const QString &accessToken);
    void connectToServer(const QString &jid, const QString &password);
    void replayMessage();
    QString subscriptionStatus(const QString &bareJid);
    int subscriptionType(const QString &bareJid);

private slots:
    void _q_connected();
    void _q_discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void _q_discoveryItemsReceived(const QXmppDiscoveryIq &disco);
    void _q_dnsLookupFinished();
    void _q_error(QXmppClient::Error error);
    void _q_hostInfoFinished(const QHostInfo &hostInfo);
    void _q_messageReceived(const QXmppMessage &message);
    void _q_timeReceived(const QXmppEntityTimeIq &time);

private:
    template<class T>
    T* getManager() {
        T* manager = findExtension<T>();
        if (!manager) {
            manager = new T;
            addExtension(manager);
        }
        return manager;
    }

    ChatClientPrivate *d;
};

#endif
