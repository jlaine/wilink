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
class QXmppDiscoveryIq;
class QXmppDiscoveryManager;
class QXmppEntityTimeIq;
class QXmppEntityTimeManager;
class QXmppSrvInfo;

class ChatClient : public QXmppClient
{
    Q_OBJECT

public:
    ChatClient(QObject *parent);
    QDateTime serverTime() const;

signals:
    void diagnosticsServerFound(const QString &diagServer);
    void mucServerFound(const QString &mucServer);
    void pubSubServerFound(const QString &pubSubServer);
    void shareServerFound(const QString &shareServer);

private slots:
    void slotConnected();
    void slotDiscoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void slotDiscoveryItemsReceived(const QXmppDiscoveryIq &disco);
    void slotTimeReceived(const QXmppEntityTimeIq &time);
    void setTurnServer(const QXmppSrvInfo &serviceInfo);
    void setTurnServer(const QHostInfo &hostInfo);

private:
    QStringList discoQueue;
    QXmppDiscoveryManager *discoManager;
    int timeOffset;
    QString timeQueue;
    QXmppEntityTimeManager *timeManager;
    quint16 m_turnPort;
};

#endif
