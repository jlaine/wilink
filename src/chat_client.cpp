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

#include <QHostInfo>

#include "QXmppCallManager.h"
#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppEntityTimeIq.h"
#include "QXmppEntityTimeManager.h"
#include "QXmppLogger.h"
#include "QXmppRosterManager.h"
#include "QXmppSrvInfo.h"
#include "QXmppTransferManager.h"

#include "chat_client.h"

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent),
    timeOffset(0)
{
    connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
    discoManager = findExtension<QXmppDiscoveryManager>();
    connect(discoManager, SIGNAL(infoReceived(QXmppDiscoveryIq)),
        this, SLOT(slotDiscoveryInfoReceived(QXmppDiscoveryIq)));
    connect(discoManager, SIGNAL(itemsReceived(QXmppDiscoveryIq)),
        this, SLOT(slotDiscoveryItemsReceived(QXmppDiscoveryIq)));

    timeManager = findExtension<QXmppEntityTimeManager>();
    connect(timeManager, SIGNAL(timeReceived(QXmppEntityTimeIq)),
        this, SLOT(slotTimeReceived(QXmppEntityTimeIq)));

    // create transfer manager
    QXmppTransferManager *transferManager = getManager<QXmppTransferManager>();
    transferManager->setSupportedMethods(QXmppTransferJob::SocksMethod);

    // create call manager
    getManager<QXmppCallManager>();
}

QString ChatClient::jid() const
{
    QXmppClient *client = (QXmppClient*)this;
    return client->configuration().jid();
}

QDateTime ChatClient::serverTime() const
{
    return QDateTime::currentDateTime().addSecs(timeOffset);
}

QXmppCallManager *ChatClient::callManager()
{
    return getManager<QXmppCallManager>();
}

QString ChatClient::diagnosticServer() const
{
    return m_diagnosticServer;
}

QXmppDiscoveryManager *ChatClient::discoveryManager()
{
    return getManager<QXmppDiscoveryManager>();
}

QString ChatClient::mucServer() const
{
    return m_mucServer;
}

QXmppRosterManager *ChatClient::rosterManager()
{
    return &QXmppClient::rosterManager();
}

QString ChatClient::shareServer() const
{
    return m_shareServer;
}

QXmppTransferManager *ChatClient::transferManager()
{
    return getManager<QXmppTransferManager>();
}

void ChatClient::slotConnected()
{
    const QString domain = configuration().domain();

    // notify jid change
    emit jidChanged(jid());

    // get server time
    timeQueue = timeManager->requestTime(domain);

    // get info for root item
    const QString id = discoManager->requestInfo(domain);
    if (!id.isEmpty())
        discoQueue.append(id);

    // lookup TURN server
    debug(QString("Looking up STUN server for domain %1").arg(domain));
    QXmppSrvInfo::lookupService("_turn._udp." + domain, this,
                                SLOT(setTurnServer(QXmppSrvInfo)));
}

void ChatClient::setTurnServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName = "turn." + configuration().domain();
    m_turnPort = 3478;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        m_turnPort = serviceInfo.records().first().port();
    }

    // lookup TURN host name
    QHostInfo::lookupHost(serverName, this, SLOT(setTurnServer(QHostInfo)));
}

void ChatClient::setTurnServer(const QHostInfo &hostInfo)
{
    if (hostInfo.addresses().isEmpty()) {
        warning(QString("Could not lookup TURN server %1").arg(hostInfo.hostName()));
        return;
    }

    QXmppCallManager *callManager = findExtension<QXmppCallManager>();
    if (callManager) {
        callManager->setTurnServer(hostInfo.addresses().first(), m_turnPort);
        callManager->setTurnUser(configuration().user());
        callManager->setTurnPassword(configuration().password());
    }
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
            m_mucServer = disco.from();
            info("Found chat room server " + m_mucServer);
            emit mucServerChanged(m_mucServer);
        }
        // check if it's a diagnostics server
        else if (id.category() == "diagnostics" &&
                 id.type() == "server")
        {
            m_diagnosticServer = disco.from();
            info("Found diagnostics server " + m_diagnosticServer);
            emit diagnosticServerChanged(m_diagnosticServer);
        }
        // check if it's a publish-subscribe server
        else if (id.category() == "pubsub" &&
                 id.type() == "service")
        {
            info("Found pubsub server " + disco.from());
            emit pubSubServerFound(disco.from());
        }
        // check if it's a SOCKS5 proxy server
        else if (id.category() == "proxy" &&
                 id.type() == "bytestreams")
        {
            info("Found bytestream proxy " + disco.from());
            QXmppTransferManager *transferManager = findExtension<QXmppTransferManager>();
            if (transferManager)
                transferManager->setProxy(disco.from());
        }
        // check if it's a file sharing server
        else if (id.category() == "store" &&
                 id.type() == "file")
        {
            m_shareServer = disco.from();
            info("Found share server " + m_shareServer);
            emit shareServerChanged(m_shareServer);
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

void ChatClient::slotTimeReceived(const QXmppEntityTimeIq &time)
{
    if (time.id() != timeQueue)
        return;
    timeQueue = QString();

    if (time.type() == QXmppIq::Result && time.utc().isValid())
        timeOffset = QDateTime::currentDateTime().secsTo(time.utc());
}

