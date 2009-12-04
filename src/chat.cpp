/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <QDebug>
#include <QList>
#include <QStringList>
#include <QSystemTrayIcon>

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"

#include "qnetio/dns.h"
#include "chat.h"

using namespace QNetIO;

Chat::Chat(QObject *parent)
    : QXmppClient(parent), systemTrayIcon(NULL)
{
    connect(this, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(handleMessage(const QXmppMessage&)));
}

void Chat::handleMessage(const QXmppMessage &msg)
{
    const QString body = msg.getBody();
    const QString from = msg.getFrom().split("@")[0];

    if (body.isEmpty())
        return;
    
    if (systemTrayIcon)
        systemTrayIcon->showMessage(from, body);
}

bool Chat::open(const QString &jid, const QString &password)
{
    QXmppConfiguration config;
    config.setResource("BocChat");

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::NONE);

    /* get user and domain */
    QStringList bits = jid.split("@");
    if (bits.size() != 2)
    {
        qWarning("Invalid JID");
        return false;
    }
    config.setUser(bits[0]);
    config.setDomain(bits[1]);

    /* get the server */
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.getDomain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.getDomain());
    }

    /* connect to server */
    config.setPasswd(password);
    connectToServer(config);
    return true;
}

void Chat::setSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    systemTrayIcon = trayIcon;
}

