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

#ifndef __WILINK_DISCOVERY_H__
#define __WILINK_DISCOVERY_H__

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QXmppClient;

#include "QXmppDiscoveryIq.h"

#include "chat_panel.h"

class Discovery : public ChatPanel
{
    Q_OBJECT

public:
    Discovery(QXmppClient *client, QWidget *parent);

private slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void goBack();
    void goForward(QListWidgetItem *item);
    void goTo();
    void slotShow();

private:
    void explore(const QXmppDiscoveryIq::Item &item);

    QXmppClient *m_client;
    QLineEdit *m_locationJid;
    QLineEdit *m_locationNode;
    QListWidget *m_listWidget;
    QPushButton *m_backButton;
    QList<QXmppDiscoveryIq::Item> m_trail;
    QStringList m_requests;
};

#endif
