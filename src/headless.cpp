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

#include <cstdlib>
#include <signal.h>

#include <QCoreApplication>
#include <QSettings>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppUtils.h"
#include "qxmpp/QXmppTransferManager.h"

#include "chat_client.h"
#include "headless.h"
#include "plugins/shares/database.h"

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

Headless::Headless(ChatSharesDatabase *db)
    : m_db(db)
{
    /* database */
    connect(m_db, SIGNAL(getFinished(QXmppShareGetIq, QXmppShareItem)),
        this, SLOT(getFinished(QXmppShareGetIq, QXmppShareItem)));
    connect(m_db, SIGNAL(searchFinished(QXmppShareSearchIq)),
        this, SLOT(searchFinished(QXmppShareSearchIq)));

    /* create configuration */
    QXmppConfiguration config;
    config.setUser("share-agent");
    config.setDomain("wireless.wifirst.fr");
    config.setPasswd("password");
    config.setHost(config.domain());
    config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
    config.setIgnoreSslErrors(false);
    config.setKeepAliveInterval(60);
    config.setKeepAliveTimeout(15);

    /* connect to server */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::StdoutLogging);

    m_client = new ChatClient(this);
#if 0
    connect(m_client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
#endif
    connect(m_client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)),
        m_db, SLOT(get(const QXmppShareGetIq&)));
    connect(m_client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)),
        m_db, SLOT(search(const QXmppShareSearchIq&)));
    connect(m_client, SIGNAL(shareServerFound(const QString&)), this, SLOT(shareServerFound(const QString&)));
    m_client->setLogger(logger);


    m_client->connectToServer(config);
}

void Headless::getFinished(const QXmppShareGetIq &iq, const QXmppShareItem &shareItem)
{
    QXmppShareGetIq responseIq(iq);

    // FIXME: for some reason, random number generation in thread is broken
    if (responseIq.type() != QXmppIq::Error)
        responseIq.setSid(generateStanzaHash());
    m_client->sendPacket(responseIq);

    // send file
    if (responseIq.type() != QXmppIq::Error)
    {
        QString filePath = m_db->filePath(shareItem.locations()[0].node());
        QXmppTransferFileInfo fileInfo;
        fileInfo.setName(shareItem.name());
        fileInfo.setDate(shareItem.fileDate());
        fileInfo.setHash(shareItem.fileHash());
        fileInfo.setSize(shareItem.fileSize());

        QFile *file = new QFile(filePath);
        file->open(QIODevice::ReadOnly);
        QXmppTransferJob *job = m_client->getTransferManager().sendFile(responseIq.to(), file, fileInfo, responseIq.sid());
        file->setParent(job);
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    }
}

void Headless::searchFinished(const QXmppShareSearchIq &responseIq)
{
    m_client->sendPacket(responseIq);
}

void Headless::shareServerFound(const QString &shareServer)
{
    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    QXmppElement nickName;
    nickName.setTagName("nickName");
    nickName.setValue("ShareAgent");
    x.appendChild(nickName);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(x);
    m_client->sendPacket(presence);
}

int main(int argc, char *argv[])
{
    /* Create application */
    QCoreApplication app(argc, argv);

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Run application */
    ChatSharesDatabase db;
    Headless headless(&db);
    return app.exec();
}

