/*
 * wDesktop
 * Copyright (C) 2009-2010 Bolloré telecom
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

#ifndef __WDESKTOP_CHAT_SHARES_H__
#define __WDESKTOP_CHAT_SHARES_H__

#include <QMap>
#include <QObject>

class QXmppClient;
class QXmppDiscoveryIq;

extern const char* ns_shares;

class ChatShares : public QObject
{
    Q_OBJECT

public:
    ChatShares(QXmppClient *client, QObject *parent);
    ~ChatShares();
    void setShareServer(const QString &server);

private slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void findFiles();

private:
    void registerWithServer();
    void unregisterFromServer();

private:
    QXmppClient *client;
    QString shareServer;
    QMap<QString, QString> sharedFiles;
};

#endif
