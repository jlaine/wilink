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

#include <QAction>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QLayout>
#include <QShortcut>
#include <QTimer>

#include "QXmppClient.h"
#include "QXmppDiscoveryManager.h"

#include "chat.h"
#include "chat_plugin.h"
#include "discovery.h"

#define DISCOVERY_ROSTER_ID "0_discovery"

class DiscoveryItem : public ChatModelItem
{
public:
    QString jid;
    QString name;
    QString node;
};

DiscoveryModel::DiscoveryModel(QObject *parent)
    : ChatModel(parent),
    m_client(0),
    m_details(false),
    m_manager(0)
{
    QHash<int, QByteArray> names = roleNames();
    names.insert(ChatModel::UserRole, "node");
    setRoleNames(names);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(100);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
}

QVariant DiscoveryModel::data(const QModelIndex &index, int role) const
{
    DiscoveryItem *item = static_cast<DiscoveryItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == ChatModel::AvatarRole)
        return QUrl("qrc:/peer.png");
    else if (role == ChatModel::NameRole)
        return item->name;
    else if (role == ChatModel::JidRole)
        return item->jid;
    else if (role == ChatModel::UserRole)
        return item->node;
    return QVariant();
}

bool DiscoveryModel::details() const
{
    return m_details;
}

void DiscoveryModel::setDetails(bool details)
{
    if (details != m_details) {
        m_details = details;
        emit detailsChanged(m_details);
    }
}

void DiscoveryModel::infoReceived(const QXmppDiscoveryIq &disco)
{
    if (!m_requests.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        DiscoveryItem *item = static_cast<DiscoveryItem*>(ptr);
        if (item->jid != disco.from() || item->node != disco.queryNode())
            continue;

        if (!disco.identities().isEmpty()) {
            item->name = disco.identities().first().name();
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
}

void DiscoveryModel::itemsReceived(const QXmppDiscoveryIq &disco)
{
    if (!m_requests.removeAll(disco.id()) ||
        disco.type() != QXmppIq::Result ||
        disco.from() != m_rootJid ||
        disco.queryNode() != m_rootNode)
        return;

    removeRows(0, rootItem->children.size());
    foreach (const QXmppDiscoveryIq::Item &item, disco.items()) {
        DiscoveryItem *ptr = new DiscoveryItem;
        ptr->jid = item.jid();
        ptr->node = item.node();
        ptr->name = item.name();
        addItem(ptr, rootItem, rootItem->children.size());

        // request information
        if (m_details) {
            const QString id = m_manager->requestInfo(item.jid(), item.node());
            if (!id.isEmpty())
                m_requests.append(id);
        }
    }
}

void DiscoveryModel::refresh()
{
    if (m_client && m_client->isConnected() && m_manager && !m_rootJid.isEmpty()) {
        const QString id = m_manager->requestItems(m_rootJid, m_rootNode);
        if (!id.isEmpty())
            m_requests.append(id);
    }
}

QString DiscoveryModel::rootJid() const
{
    return m_rootJid;
}

void DiscoveryModel::setRootJid(const QString &rootJid)
{
    if (rootJid != m_rootJid) {
        m_rootJid = rootJid;
        emit rootJidChanged(m_rootJid);
        m_timer->start();
    }
}

QString DiscoveryModel::rootNode() const
{
    return m_rootNode;
}

void DiscoveryModel::setRootNode(const QString &rootNode)
{
    if (rootNode != m_rootNode) {
        m_rootNode = rootNode;
        emit rootNodeChanged(m_rootNode);
        m_timer->start();
    }
}

QXmppDiscoveryManager *DiscoveryModel::manager() const
{
    return m_manager;
}

void DiscoveryModel::setManager(QXmppDiscoveryManager *manager)
{
    if (manager != m_manager) {
        m_manager = manager;
        if (m_manager) {
            bool check;

            m_client = qobject_cast<QXmppClient*>(m_manager->parent());
            Q_ASSERT(m_client);
            check = connect(m_client, SIGNAL(connected()),
                            this, SLOT(refresh()));
            Q_ASSERT(check);

            check = connect(m_manager, SIGNAL(infoReceived(QXmppDiscoveryIq)),
                            this, SLOT(infoReceived(QXmppDiscoveryIq)));
            Q_ASSERT(check);

            check = connect(m_manager, SIGNAL(itemsReceived(QXmppDiscoveryIq)),
                            this, SLOT(itemsReceived(QXmppDiscoveryIq)));
            Q_ASSERT(check);
        }
        emit managerChanged(m_manager);
        m_timer->start();
    }
}

