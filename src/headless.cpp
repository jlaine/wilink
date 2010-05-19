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

#include "chat_client.h"
#include "headless.h"

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

Headless::Headless()
{
    /* create configuration */
    QXmppConfiguration config;
    config.setUser("user");
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
    m_client->setLogger(logger);
    m_client->connectToServer(config);
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
    Headless headless;
    return app.exec();
}

