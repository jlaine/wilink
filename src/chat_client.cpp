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

#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppLogger.h"
#include "QXmppTransferManager.h"

#include "chat_client.h"

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent)
{
    discoManager = findExtension<QXmppDiscoveryManager>();

    connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(discoManager, SIGNAL(infoReceived(const QXmppDiscoveryIq&)),
        this, SLOT(slotDiscoveryInfoReceived(const QXmppDiscoveryIq&)));
    connect(discoManager, SIGNAL(itemsReceived(const QXmppDiscoveryIq&)),
        this, SLOT(slotDiscoveryItemsReceived(const QXmppDiscoveryIq&)));
}

void ChatClient::slotConnected()
{
    // get info for root item
    const QString id = discoManager->requestInfo(configuration().domain());
    if (!id.isEmpty())
        discoQueue.append(id);
}

void ChatClient::slotDiscoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (!discoQueue.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    foreach (const QXmppDiscoveryIq::Identity &id, disco.identities())
    {
        // check if it's a conference server
        if (id.category() == "conference" &&
            id.type() == "text")
        {
            emit logMessage(QXmppLogger::InformationMessage, "Found chat room server " + disco.from());
            emit mucServerFound(disco.from());
        }
        // check if it's a diagnostics server
        else if (id.category() == "diagnostics" &&
                 id.type() == "server")
        {
            emit logMessage(QXmppLogger::InformationMessage, "Found diagnostics server " + disco.from());
            emit diagnosticsServerFound(disco.from());
        }
        // check if it's a publish-subscribe server
        else if (id.category() == "pubsub" &&
                 id.type() == "service")
        {
            emit logMessage(QXmppLogger::InformationMessage, "Found pubsub server " + disco.from());
            emit pubSubServerFound(disco.from());
        }
        // check if it's a SOCKS5 proxy server
        else if (id.category() == "proxy" &&
                 id.type() == "bytestreams")
        {
            emit logMessage(QXmppLogger::InformationMessage, "Found bytestream proxy " + disco.from());
            transferManager().setProxy(disco.from());
        }
        // check if it's a file sharing server
        else if (id.category() == "store" &&
                 id.type() == "file")
        {
            emit logMessage(QXmppLogger::InformationMessage, "Found share server " + disco.from());
            emit shareServerFound(disco.from());
        }
    }

    // if it's the root server, ask for items
    if (disco.from() == configuration().domain() && disco.queryNode().isEmpty())
    {
        const QString id = discoManager->requestItems(disco.from(), disco.queryNode());
        if (!id.isEmpty())
            discoQueue.append(id);
    }
}

void ChatClient::slotDiscoveryItemsReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (!discoQueue.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    if (disco.from() == configuration().domain() &&
        disco.queryNode().isEmpty())
    {
        // root items
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            if (!item.jid().isEmpty() && item.node().isEmpty())
            {
                // get info for item
                const QString id = discoManager->requestInfo(item.jid(), item.node());
                if (!id.isEmpty())
                    discoQueue.append(id);
            }
        }
    }
}

