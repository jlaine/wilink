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

#ifndef __WILINK_CHAT_STATUS_H__
#define __WILINK_CHAT_STATUS_H__

#include <QComboBox>

class Idle;
class QXmppClient;
class QSystemTrayIcon;

class ChatStatus : public QComboBox
{
    Q_OBJECT

public:
    ChatStatus(QXmppClient *client);
    ~ChatStatus();

private slots:
    void connected();
    void disconnected();
    void secondsIdle(int secs);
    void statusChanged(int currentIndex);

private:
    Idle *m_idle;
    QXmppClient *m_client;
    bool m_autoAway;
    bool m_freezeStatus;
    QSystemTrayIcon *trayIcon;
};

#endif
