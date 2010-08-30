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

#include <QLayout>
#include <QListWidget>
#include <QShortcut>

#include "QXmppDiscoveryIq.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "discovery.h"

enum Roles
{
    JidRole = Qt::UserRole,
    NodeRole,
};

Discovery::Discovery(QXmppClient *client, QWidget *parent)
    : ChatPanel(parent),
    m_client(client)
{
    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addItem(headerLayout());

    m_listWidget = new QListWidget;
    layout->addWidget(m_listWidget);

    setLayout(layout);
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Service discovery"));

    /* connect signals */
    bool check = connect(this, SIGNAL(showPanel()),
        this, SLOT(slotShow()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
        this, SIGNAL(unregisterPanel()));
    Q_ASSERT(check);

    check = connect(m_client, SIGNAL(discoveryIqReceived(QXmppDiscoveryIq)),
        this, SLOT(discoveryIqReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(itemDoubleClicked(QListWidgetItem*)));
    Q_ASSERT(check);
}

void Discovery::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (!m_requests.removeAll(disco.id()))
        return;
   
    if (disco.type() == QXmppIq::Result)
    {
        if (disco.queryType() == QXmppDiscoveryIq::ItemsQuery)
        {
            m_listWidget->clear();
            foreach (const QXmppDiscoveryIq::Item &item, disco.items())
            {
                // insert item
                QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), item.jid());
                wdgItem->setData(JidRole, item.jid());
                wdgItem->setData(NodeRole, item.node());
                m_listWidget->addItem(wdgItem);

                // request information
                QXmppDiscoveryIq iq;
                iq.setTo(item.jid());
                iq.setType(QXmppIq::Get);
                iq.setQueryNode(item.node());
                iq.setQueryType(QXmppDiscoveryIq::InfoQuery);
                m_requests.append(iq.id());
                m_client->sendPacket(iq);
            }
        }
        else if (disco.queryType() == QXmppDiscoveryIq::InfoQuery)
        {
            for (int i = 0; i < m_listWidget->count(); ++i)
            {
                QListWidgetItem *wdgItem = m_listWidget->item(i);
                if (wdgItem->data(JidRole).toString() != disco.from() ||
                    wdgItem->data(NodeRole).toString() != disco.queryNode())
                    continue;

                if (!disco.identities().isEmpty())
                    wdgItem->setText(disco.identities().first().name() + "\n" + wdgItem->text());
            }
        }
    }
}

void Discovery::itemDoubleClicked(QListWidgetItem *item)
{
    QXmppDiscoveryIq iq;
    iq.setTo(item->data(JidRole).toString());
    iq.setType(QXmppIq::Get);
    iq.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    iq.setQueryNode(item->data(NodeRole).toString());
    m_requests.append(iq.id());
    m_client->sendPacket(iq);
}

void Discovery::slotShow()
{
    QXmppDiscoveryIq iq;
    iq.setTo(m_client->configuration().domain());
    iq.setType(QXmppIq::Get);
    iq.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    m_requests.append(iq.id());
    m_client->sendPacket(iq);
}

// PLUGIN

class DiscoveryPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool DiscoveryPlugin::initialize(Chat *chat)
{
    /* register panel */
    Discovery *discovery = new Discovery(chat->client(), chat);
    discovery->setObjectName("discovery");
    chat->addPanel(discovery);

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_B), chat);
    connect(shortcut, SIGNAL(activated()), discovery, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(discovery, DiscoveryPlugin)
