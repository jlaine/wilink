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

#include <QDomElement>

#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMucIq.h"
#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"

#include "chat_client.h"

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent)
{
    connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(this, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)),
        this, SLOT(slotDiscoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(this, SIGNAL(elementReceived(const QDomElement&, bool&)),
        this, SLOT(slotElementReceived(const QDomElement&, bool&)));
}

void ChatClient::slotConnected()
{
    /* discover services */
    QXmppDiscoveryIq disco;
    disco.setTo(getConfiguration().domain());
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    sendPacket(disco);
}

void ChatClient::slotDiscoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (disco.type() != QXmppIq::Result)
        return;

    if (disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == getConfiguration().domain())
    {
        // root items
        discoQueue.clear();
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            if (!item.jid().isEmpty() && item.node().isEmpty())
            {
                discoQueue.append(item.jid());
                // get info for item
                QXmppDiscoveryIq info;
                info.setQueryType(QXmppDiscoveryIq::InfoQuery);
                info.setTo(item.jid());
                sendPacket(info);
            }
        }
    }
    else if (disco.queryType() == QXmppDiscoveryIq::InfoQuery &&
             discoQueue.contains(disco.from()))
    {
        discoQueue.removeAll(disco.from());
        // check if it's a conference server
        foreach (const QXmppDiscoveryIq::Identity &id, disco.identities())
        {
            if (id.category() == "conference" &&
                id.type() == "text")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found chat room server " + disco.from());
                emit mucServerFound(disco.from());
            }
            else if (id.category() == "proxy" &&
                     id.type() == "bytestreams")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found bytestream proxy " + disco.from());
                getTransferManager().setProxy(disco.from());
            }
            else if (id.category() == "store" &&
                     id.type() == "file")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found share server " + disco.from());
                emit shareServerFound(disco.from());
            }
        }
    }
}

void ChatClient::slotElementReceived(const QDomElement &element, bool &handled)
{
    if (element.tagName() == "iq")
    {
        if (QXmppShareGetIq::isShareGetIq(element))
        {
            QXmppShareGetIq getIq;
            getIq.parse(element);
            emit shareGetIqReceived(getIq);
            handled = true;
        }
        else if (QXmppShareSearchIq::isShareSearchIq(element))
        {
            QXmppShareSearchIq searchIq;
            searchIq.parse(element);
            emit shareSearchIqReceived(searchIq);
            handled = true;
        }
        else if (QXmppMucAdminIq::isMucAdminIq(element))
        {
            QXmppMucAdminIq mucIq;
            mucIq.parse(element);
            emit mucAdminIqReceived(mucIq);
            handled = true;
        }
    }
}

