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

#ifndef __WILINK_CHAT_CLIENT_H__
#define __WILINK_CHAT_CLIENT_H__

#include "QXmppClient.h"

class QDateTime;
class QHostInfo;
class QXmppArchiveManager;
class QXmppCallManager;
class QXmppDiscoveryIq;
class QXmppDiscoveryManager;
class QXmppEntityTimeIq;
class QXmppEntityTimeManager;
class QXmppMucManager;
class QXmppSrvInfo;
class QXmppTransferManager;

class ChatClient : public QXmppClient
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid NOTIFY jidChanged)
    Q_PROPERTY(QXmppLogger* logger READ logger CONSTANT)
    Q_PROPERTY(QXmppArchiveManager* archiveManager READ archiveManager CONSTANT)
    Q_PROPERTY(QXmppCallManager* callManager READ callManager CONSTANT)
    Q_PROPERTY(QString diagnosticServer READ diagnosticServer NOTIFY diagnosticServerChanged)
    Q_PROPERTY(QXmppDiscoveryManager* discoveryManager READ discoveryManager CONSTANT)
    Q_PROPERTY(QXmppMucManager* mucManager READ mucManager CONSTANT)
    Q_PROPERTY(QString mucServer READ mucServer NOTIFY mucServerChanged)
    Q_PROPERTY(QXmppRosterManager* rosterManager READ rosterManager CONSTANT)
    Q_PROPERTY(QString shareServer READ shareServer NOTIFY shareServerChanged)
    Q_PROPERTY(QXmppTransferManager* transferManager READ transferManager CONSTANT)

public:
    ChatClient(QObject *parent);
    QString jid() const;
    QDateTime serverTime() const;

    QXmppArchiveManager *archiveManager();
    QXmppCallManager *callManager();
    QString diagnosticServer() const;
    QXmppDiscoveryManager *discoveryManager();
    QXmppMucManager *mucManager();
    QString mucServer() const;
    QXmppRosterManager *rosterManager();
    QString shareServer() const;
    QXmppTransferManager* transferManager();

signals:
    void jidChanged(const QString &jid);
    void diagnosticServerChanged(const QString &diagnosticServer);
    void mucServerChanged(const QString &mucServer);
    void shareServerChanged(const QString &shareServer);

private slots:
    void slotConnected();
    void slotDiscoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void slotDiscoveryItemsReceived(const QXmppDiscoveryIq &disco);
    void slotTimeReceived(const QXmppEntityTimeIq &time);
    void setTurnServer(const QXmppSrvInfo &serviceInfo);
    void setTurnServer(const QHostInfo &hostInfo);

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

    QString m_diagnosticServer;
    QStringList discoQueue;
    QXmppDiscoveryManager *discoManager;
    QString m_mucServer;
    QString m_shareServer;
    int timeOffset;
    QString timeQueue;
    QXmppEntityTimeManager *timeManager;
    quint16 m_turnPort;
};

#endif
