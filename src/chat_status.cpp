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

#include <QApplication>
#include <QPainter>

#include "QXmppClient.h"
#include "QXmppPresence.h"

#include "application.h"
#include "chat_status.h"
#include "idle/idle.h"

#define AWAY_TIME 300 // set away after 5 mn

enum StatusIndexes {
    AvailableIndex = 0,
    BusyIndex = 1,
    AwayIndex = 2,
    OfflineIndex = 3,
};

ChatStatus::ChatStatus(QXmppClient *client)
    : m_client(client),
    m_autoAway(false),
    m_freezeStatus(false)
{
    bool check;

    addItem(QIcon(":/contact-available.png"), tr("Available"));
    addItem(QIcon(":/contact-busy.png"), tr("Busy"));
    addItem(QIcon(":/contact-away.png"), tr("Away"));
    addItem(QIcon(":/contact-offline.png"), tr("Offline"));
    setCurrentIndex(OfflineIndex);

    /* set up idle monitor */
    m_idle = new Idle;
    check = connect(m_idle, SIGNAL(secondsIdle(int)),
                    this, SLOT(secondsIdle(int)));
    Q_ASSERT(check);
    m_idle->start();

    check = connect(m_client, SIGNAL(connected()),
                    this, SLOT(connected()));
    Q_ASSERT(check);

    check = connect(m_client, SIGNAL(disconnected()),
                    this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(statusChanged(int)));
    Q_ASSERT(check);
}

ChatStatus::~ChatStatus()
{
    delete m_idle;
}

/** Handle successful connection to the chat server.
 */
void ChatStatus::connected()
{
    QXmppPresence::Status::Type statusType = m_client->clientPresence().status().type();
    if (statusType == QXmppPresence::Status::Away)
        setCurrentIndex(AwayIndex);
    else if (statusType == QXmppPresence::Status::DND)
        setCurrentIndex(BusyIndex);
    else
        setCurrentIndex(AvailableIndex);
}

/** Handle disconnection from the chat server.
 */
void ChatStatus::disconnected()
{
    m_freezeStatus = true;
    setCurrentIndex(OfflineIndex);
    m_freezeStatus = false;
}

/** Handle user inactivity.
 *
 * @param secs
 */
void ChatStatus::secondsIdle(int secs)
{
    if (secs >= AWAY_TIME)
    {
        if (currentIndex() == AvailableIndex)
        {
            setCurrentIndex(AwayIndex);
            m_autoAway = true;
        }
    } else if (m_autoAway) {
        setCurrentIndex(AvailableIndex);
    }
}

void ChatStatus::statusChanged(int currentIndex)
{
#ifdef USE_SYSTRAY
    QIcon icon;
    QLinearGradient gradient(0, 0, 0, 32);
    QPainter painterIcon;
    QPixmap pixmapIcon(":/wiLink.png");
#endif

    // don't change client presence when the status change
    // was due to a disconnection from the server
    if (m_freezeStatus)
        return;

    m_autoAway = false;
    QXmppPresence presence = m_client->clientPresence();
    if (currentIndex == AvailableIndex)
    {
        presence.setType(QXmppPresence::Available);
        presence.status().setType(QXmppPresence::Status::Online);
#ifdef USE_SYSTRAY
        gradient.setColorAt(0, QColor(0, 150, 0));
        gradient.setColorAt(1, QColor(0, 255, 0));

        painterIcon.begin(&pixmapIcon);
        painterIcon.setPen(QPen(QBrush(QColor(0, 100, 0)), 1));
        painterIcon.setBrush(QBrush(gradient));
        painterIcon.drawEllipse(15, 15, 15, 15);
        painterIcon.end();

        wApp->trayIcon()->setIcon(QIcon(pixmapIcon));
#endif
    }
    else if (currentIndex == AwayIndex)
    {
        presence.setType(QXmppPresence::Available);
        presence.status().setType(QXmppPresence::Status::Away);
#ifdef USE_SYSTRAY
        gradient.setColorAt(0, QColor(210, 140, 0));
        gradient.setColorAt(1, QColor(255, 200, 0));

        painterIcon.begin(&pixmapIcon);
        painterIcon.setPen(QPen(QBrush(QColor(200, 100, 0)), 1));
        painterIcon.setBrush(QBrush(gradient));
        painterIcon.drawEllipse(15, 15, 15, 15);
        painterIcon.end();

        wApp->trayIcon()->setIcon(QIcon(pixmapIcon));
#endif
    }
    else if (currentIndex == BusyIndex)
    {
        presence.setType(QXmppPresence::Available);
        presence.status().setType(QXmppPresence::Status::DND);
#ifdef USE_SYSTRAY
        gradient.setColorAt(0, QColor(255, 0, 0));
        gradient.setColorAt(1, QColor(255, 100, 100));

        painterIcon.begin(&pixmapIcon);
        painterIcon.setPen(QPen(QBrush(QColor(100, 0, 0)), 1));
        painterIcon.setBrush(QBrush(gradient));
        painterIcon.drawEllipse(15, 15, 15, 15);
        painterIcon.end();

        wApp->trayIcon()->setIcon(QIcon(pixmapIcon));
#endif
    }
    else if (currentIndex == OfflineIndex)
    {
        presence.setType(QXmppPresence::Unavailable);
        presence.status().setType(QXmppPresence::Status::Offline);
#ifdef USE_SYSTRAY
        wApp->trayIcon()->setIcon(QIcon(":/wiLink.png"));
#endif
    }
    m_client->setClientPresence(presence);
}
