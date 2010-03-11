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

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppDiscoveryIq.h"

#include "chat_shares.h"

const char* ns_shares = "http://wifirst.net/protocol/shares";

ChatShares::ChatShares(QXmppClient *xmppClient, QObject *parent)
    : QObject(parent), client(xmppClient)
{
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
}

void ChatShares::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() == QXmppIq::Get &&
        disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == shareServer)
    {
        qDebug() << "GOT A QUERY";
        QXmppDiscoveryIq iq;
        iq.setId(disco.id());
        iq.setTo(disco.from());
        iq.setType(QXmppIq::Result);
        iq.setQueryType(disco.queryType());

        QList<QXmppElement> items;
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("jid", client->getConfiguration().jid());
        item.setAttribute("node", "azerty");
        item.setAttribute("name", "test_file.txt");
        items.append(item);

        iq.setQueryItems(items);
        client->sendPacket(iq);
    }
}

void ChatShares::setShareServer(const QString &server)
{
    shareServer = server;

    // register with server
    QList<QXmppElement> extensions;
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);
    extensions.append(x);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(extensions);
    client->sendPacket(presence);
}

