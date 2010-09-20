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

#include <QCoreApplication>

#include "QXmppUtils.h"

#include "chat_client.h"
#include "chat_roster.h"
#include "chat_utils.h"

#include "window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/chat.png"));

    m_client = new ChatClient(this);
    m_rosterModel =  new ChatRosterModel(m_client, this);
    connect(m_rosterModel, SIGNAL(rosterReady()), this, SLOT(resizeContacts()));
    connect(m_rosterModel, SIGNAL(pendingMessages(int)), this, SLOT(pendingMessages(int)));
}

bool MainWindow::open(const QString &jid, const QString &password)
{
    QXmppConfiguration config;
    config.setResource(qApp->applicationName());

    /* get user and domain */
    if (!isBareJid(jid))
    {
        qWarning("Cannot connect to chat server using invalid JID");
        return false;
    }
    config.setJid(jid);
    config.setPassword(password);

    /* set security parameters */
    if (config.domain() == QLatin1String("wifirst.net"))
    {
        config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
        config.setIgnoreSslErrors(false);
    }

    /* set keep alive */
    config.setKeepAliveTimeout(15);

    /* connect to server */
    m_client->connectToServer(config);
    return true;
}

