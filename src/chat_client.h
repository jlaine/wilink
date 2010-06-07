/*
 * wiLink
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

#include "qxmpp/QXmppClient.h"

class QXmppMucAdminIq;
class QXmppMucOwnerIq;
class QXmppShareGetIq;
class QXmppShareSearchIq;

class ChatClient : public QXmppClient
{
    Q_OBJECT

public:
    ChatClient(QObject *parent);

signals:
    void mucAdminIqReceived(const QXmppMucAdminIq &iq);
    void mucServerFound(const QString &mucServer);
    void shareGetIqReceived(const QXmppShareGetIq &iq);
    void shareSearchIqReceived(const QXmppShareSearchIq &iq);
    void shareServerFound(const QString &shareServer);

private slots:
    void slotConnected();
    void slotDiscoveryIqReceived(const QXmppDiscoveryIq &disco);
    void slotElementReceived(const QDomElement &element, bool &handled);

private:
    QStringList discoQueue;
};


