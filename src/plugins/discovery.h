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

#ifndef __WILINK_DISCOVERY_H__
#define __WILINK_DISCOVERY_H__

class QAction;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QXmppClient;
class QXmppDiscoveryManager;

#include "QXmppDiscoveryIq.h"

#include "chat_panel.h"
#include "chat_model.h"

class DiscoveryModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(QXmppDiscoveryManager* manager READ manager WRITE setManager NOTIFY managerChanged)

public:
    DiscoveryModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QXmppDiscoveryManager *manager() const;
    void setManager(QXmppDiscoveryManager *manager);

signals:
    void managerChanged(QXmppDiscoveryManager *manager);

private slots:
    void itemsReceived(const QXmppDiscoveryIq &disco);

private:
    QXmppDiscoveryManager *m_manager;
};

/** The DiscoveryPanel class represents a panel for displaying service
 *  discovery results.
 */
class DiscoveryPanel : public ChatPanel
{
    Q_OBJECT

public:
    DiscoveryPanel(QXmppClient *client, QWidget *parent);

private slots:
    void discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void discoveryItemsReceived(const QXmppDiscoveryIq &disco);
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
    QAction *m_backAction;
    QList<QXmppDiscoveryIq::Item> m_trail;
    QXmppDiscoveryManager *m_manager;
    QStringList m_requests;
};

#endif
