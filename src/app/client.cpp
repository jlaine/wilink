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

#include <QCoreApplication>
#include <QHostInfo>
#include "qdnslookup.h"

#include "QXmppArchiveManager.h"
#include "QXmppBookmarkManager.h"
#include "QXmppCallManager.h"
#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppEntityTimeIq.h"
#include "QXmppEntityTimeManager.h"
#include "QXmppLogger.h"
#include "QXmppMessage.h"
#include "QXmppMucManager.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"

#include "client.h"
#include "diagnostics.h"

static QList<ChatClient*> chatClients;

class ChatClientPrivate
{
public:
    ChatClientPrivate() : timeOffset(0)
    {}

    QString diagnosticServer;
    QStringList discoQueue;
    QXmppDiscoveryManager *discoManager;
    QDnsLookup dns;
    QXmppMessage lastMessage;
    QString mucServer;
    QString shareServer;
    int timeOffset;
    QString timeQueue;
    QXmppEntityTimeManager *timeManager;
    quint16 turnPort;
};

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent)
    , d(new ChatClientPrivate)
{
    bool check;
    Q_UNUSED(check);

    check = connect(this, SIGNAL(connected()),
                    this, SLOT(_q_connected()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(error(QXmppClient::Error)),
            this, SLOT(_q_error(QXmppClient::Error)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(messageReceived(QXmppMessage)),
            this, SLOT(_q_messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    // DNS lookups
    check = connect(&d->dns, SIGNAL(finished()),
                    this, SLOT(_q_dnsLookupFinished()));
    Q_ASSERT(check);

    // service discovery
    d->discoManager = findExtension<QXmppDiscoveryManager>();
    check = connect(d->discoManager, SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(_q_discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(d->discoManager, SIGNAL(itemsReceived(QXmppDiscoveryIq)),
                    this, SLOT(_q_discoveryItemsReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    // server time
    d->timeManager = findExtension<QXmppEntityTimeManager>();
    check = connect(d->timeManager, SIGNAL(timeReceived(QXmppEntityTimeIq)),
                    this, SLOT(_q_timeReceived(QXmppEntityTimeIq)));
    Q_ASSERT(check);

    // file transfers
    transferManager()->setSupportedMethods(QXmppTransferJob::SocksMethod);

    // multimedia calls
    callManager();

    // diagnostics
    diagnosticManager();

    chatClients.append(this);
}

ChatClient::~ChatClient()
{
    chatClients.removeAll(this);
    delete d;
}

void ChatClient::connectToServer(const QString &jid, const QString &password)
{
    QXmppConfiguration config;
    config.setResource(qApp->applicationName());
    config.setJid(jid);
    config.setPassword(password);
    if (config.domain() == QLatin1String("wifirst.net"))
    {
        config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
        config.setIgnoreSslErrors(false);
    }
    QXmppClient::connectToServer(config);
}

QList<ChatClient*> ChatClient::instances()
{
    return chatClients;
}

QString ChatClient::jid() const
{
    QXmppClient *client = (QXmppClient*)this;
    return client->configuration().jid();
}

QDateTime ChatClient::serverTime() const
{
    return QDateTime::currentDateTime().addSecs(d->timeOffset);
}

int ChatClient::statusType() const
{
    return clientPresence().status().type();
}

void ChatClient::setStatusType(int statusType)
{
    const QXmppPresence::Status::Type newType = static_cast<QXmppPresence::Status::Type>(statusType);
    QXmppPresence presence = clientPresence();
    if (newType != presence.status().type()) {
        if (newType == QXmppPresence::Status::Offline)
            presence.setType(QXmppPresence::Unavailable);
        else
            presence.setType(QXmppPresence::Available);
        presence.status().setType(newType);
        setClientPresence(presence);

        emit statusTypeChanged(newType);
    }
}

/** Returns the current subscription type for the given contact.
 *
 * @param bareJid
 */
int ChatClient::subscriptionType(const QString &bareJid)
{
    return rosterManager()->getRosterEntry(bareJid).subscriptionType();
}

QXmppArchiveManager *ChatClient::archiveManager()
{
    return getManager<QXmppArchiveManager>();
}

QXmppBookmarkManager *ChatClient::bookmarkManager()
{
    return getManager<QXmppBookmarkManager>();
}

QXmppCallManager *ChatClient::callManager()
{
    return getManager<QXmppCallManager>();
}

DiagnosticManager *ChatClient::diagnosticManager()
{
    return getManager<DiagnosticManager>();
}

QString ChatClient::diagnosticServer() const
{
    return d->diagnosticServer;
}

QXmppDiscoveryManager *ChatClient::discoveryManager()
{
    return getManager<QXmppDiscoveryManager>();
}

QXmppMucManager *ChatClient::mucManager()
{
    return getManager<QXmppMucManager>();
}

QString ChatClient::mucServer() const
{
    return d->mucServer;
}

QXmppRosterManager *ChatClient::rosterManager()
{
    return &QXmppClient::rosterManager();
}

QString ChatClient::shareServer() const
{
    return d->shareServer;
}

QXmppTransferManager *ChatClient::transferManager()
{
    return getManager<QXmppTransferManager>();
}

void ChatClient::replayMessage()
{
    emit QXmppClient::messageReceived(d->lastMessage);
}

void ChatClient::_q_connected()
{
    const QString domain = configuration().domain();

    // notify jid change
    emit jidChanged(jid());

    // get server time
    d->timeQueue = d->timeManager->requestTime(domain);

    // get info for root item
    const QString id = d->discoManager->requestInfo(domain);
    if (!id.isEmpty())
        d->discoQueue.append(id);

    // lookup TURN server
    debug(QString("Looking up STUN server for domain %1").arg(domain));
    d->dns.setType(QDnsLookup::SRV);
    d->dns.setName("_turn._udp." + domain);
    d->dns.lookup();
}

void ChatClient::_q_dnsLookupFinished()
{
    QString serverName;

    if (d->dns.error() == QDnsLookup::NoError &&
        !d->dns.serviceRecords().isEmpty()) {
        serverName = d->dns.serviceRecords().first().target();
        d->turnPort = d->dns.serviceRecords().first().port();
    } else {
        serverName = "turn." + configuration().domain();
        d->turnPort = 3478;
    }

    // lookup TURN host name
    QHostInfo::lookupHost(serverName, this, SLOT(_q_hostInfoFinished(QHostInfo)));
}

void ChatClient::_q_hostInfoFinished(const QHostInfo &hostInfo)
{
    if (hostInfo.addresses().isEmpty()) {
        warning(QString("Could not lookup TURN server %1").arg(hostInfo.hostName()));
        return;
    }

    QXmppCallManager *callManager = findExtension<QXmppCallManager>();
    if (callManager) {
        callManager->setTurnServer(hostInfo.addresses().first(), d->turnPort);
        callManager->setTurnUser(configuration().user());
        callManager->setTurnPassword(configuration().password());
    }
}

void ChatClient::_q_discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (!d->discoQueue.removeAll(disco.id()) ||
        disco.type() != QXmppIq::Result)
        return;

    foreach (const QXmppDiscoveryIq::Identity &id, disco.identities()) {
        // check if it's a conference server
        if (id.category() == QLatin1String("conference") &&
            id.type() == QLatin1String("text")) {
            d->mucServer = disco.from();
            info("Found chat room server " + d->mucServer);
            emit mucServerChanged(d->mucServer);
        }
        // check if it's a diagnostics server
        else if (id.category() == QLatin1String("diagnostics") &&
                 id.type() == QLatin1String("server")) {
            d->diagnosticServer = disco.from();
            info("Found diagnostics server " + d->diagnosticServer);
            emit diagnosticServerChanged(d->diagnosticServer);
        }
#if 0
        // check if it's a publish-subscribe server
        else if (id.category() == QLatin1String("pubsub") &&
                 id.type() == QLatin1String("service")) {
            d->pubSubServer = disco.from();
            info("Found pubsub server " + d->pubSubServer);
            emit pubSubServerChanged(d->pubSubServer);
        }
#endif
        // check if it's a SOCKS5 proxy server
        else if (id.category() == QLatin1String("proxy") &&
                 id.type() == QLatin1String("bytestreams")) {
            info("Found bytestream proxy " + disco.from());
            QXmppTransferManager *transferManager = findExtension<QXmppTransferManager>();
            if (transferManager)
                transferManager->setProxy(disco.from());
        }
        // check if it's a file sharing server
        else if (id.category() == QLatin1String("store") &&
                 id.type() == QLatin1String("file")) {
            d->shareServer = disco.from();
            info("Found share server " + d->shareServer);
            emit shareServerChanged(d->shareServer);
        }
    }

    // if it's the root server, ask for items
    if (disco.from() == configuration().domain() && disco.queryNode().isEmpty()) {
        const QString id = d->discoManager->requestItems(disco.from(), disco.queryNode());
        if (!id.isEmpty())
            d->discoQueue.append(id);
    }
}

void ChatClient::_q_discoveryItemsReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (!d->discoQueue.removeAll(disco.id()) ||
        disco.type() != QXmppIq::Result)
        return;

    if (disco.from() == configuration().domain() &&
        disco.queryNode().isEmpty()) {
        // root items
        foreach (const QXmppDiscoveryIq::Item &item, disco.items()) {
            if (!item.jid().isEmpty() && item.node().isEmpty()) {
                // get info for item
                const QString id = d->discoManager->requestInfo(item.jid(), item.node());
                if (!id.isEmpty())
                    d->discoQueue.append(id);
            }
        }
    }
}

void ChatClient::_q_error(QXmppClient::Error error)
{
    if (error == QXmppClient::XmppStreamError) {
        if (xmppStreamError() == QXmppStanza::Error::Conflict) {
            emit conflictReceived();
        } else if (xmppStreamError() == QXmppStanza::Error::NotAuthorized) {
            emit authenticationFailed();
        }
    }
}

void ChatClient::_q_messageReceived(const QXmppMessage &message)
{
    if (message.type() == QXmppMessage::Chat && !message.body().isEmpty()) {
        d->lastMessage = message;
        emit messageReceived(message.from());
    }
}

void ChatClient::_q_timeReceived(const QXmppEntityTimeIq &time)
{
    if (time.id() != d->timeQueue)
        return;
    d->timeQueue = QString();

    if (time.type() == QXmppIq::Result && time.utc().isValid())
        d->timeOffset = QDateTime::currentDateTime().secsTo(time.utc());
}

