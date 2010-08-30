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

#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "discovery.h"

enum Roles
{
    JidRole = Qt::UserRole,
    NodeRole,
    NameRole,
};

static QString itemText(QListWidgetItem *item)
{
    const QString jid = item->data(JidRole).toString();
    const QString node = item->data(NodeRole).toString();
    const QString name = item->data(NameRole).toString();

    QString text;
    if (!name.isEmpty())
        text += "<b>" + name + "</b><br/>";
    text += jid;
    if (!node.isEmpty())
        text += QString(" (%1)").arg(node);
    return text;
}

Discovery::Discovery(QXmppClient *client, QWidget *parent)
    : ChatPanel(parent),
    m_client(client)
{
    bool check;

    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addItem(headerLayout());

    m_listWidget = new QListWidget;
    m_listWidget->setIconSize(QSize(32, 32));
    layout->addWidget(m_listWidget);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addStretch();

    m_backButton = new QPushButton(tr("Go back"));
    m_backButton->setIcon(QIcon(":/back.png"));
    m_backButton->setEnabled(false);
    hbox->addWidget(m_backButton);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Service discovery"));


    /* connect signals */
    check = connect(this, SIGNAL(showPanel()),
        this, SLOT(slotShow()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
        this, SIGNAL(unregisterPanel()));
    Q_ASSERT(check);

    check = connect(m_client, SIGNAL(discoveryIqReceived(QXmppDiscoveryIq)),
        this, SLOT(discoveryIqReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(m_backButton, SIGNAL(clicked()),
        this, SLOT(goBack()));
    Q_ASSERT(check);

    check = connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(goForward(QListWidgetItem*)));
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
            // check the results are for the current item
            if (m_trail.isEmpty() ||
                m_trail.last().jid() != disco.from() ||
                m_trail.last().node() != disco.queryNode())
                return;

            foreach (const QXmppDiscoveryIq::Item &item, disco.items())
            {
                // insert item
                QListWidgetItem *wdgItem = new QListWidgetItem;
                wdgItem->setIcon(QIcon(":/chat.png"));
                wdgItem->setData(JidRole, item.jid());
                wdgItem->setData(NodeRole, item.node());
                wdgItem->setData(NameRole, item.name());
                QLabel *label = new QLabel(itemText(wdgItem));
                m_listWidget->addItem(wdgItem);
                m_listWidget->setItemWidget(wdgItem, label);

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
                {
                    wdgItem->setData(NameRole, disco.identities().first().name());
                    QLabel *label = qobject_cast<QLabel*>(m_listWidget->itemWidget(wdgItem));
                    if (label)
                        label->setText(itemText(wdgItem));
                }
            }
        }
    }
}

void Discovery::explore(const QXmppDiscoveryIq::Item &item)
{
    // update window title
    QString extra = item.jid();
    if (!item.node().isEmpty())
        extra += QString(" (%1)").arg(item.node());
    setWindowExtra(extra);
    m_listWidget->clear();

    // request items
    QXmppDiscoveryIq iq;
    iq.setTo(item.jid());
    iq.setType(QXmppIq::Get);
    iq.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    iq.setQueryNode(item.node());
    m_requests.append(iq.id());
    m_client->sendPacket(iq);
}

void Discovery::goBack()
{
    if (m_trail.size() < 2)
        return;

    // pop location
    m_trail.pop_back();
    if (m_trail.size() < 2)
        m_backButton->setEnabled(false);
    explore(m_trail.last());
}

void Discovery::goForward(QListWidgetItem *wdgItem)
{
    QXmppDiscoveryIq::Item item;
    item.setJid(wdgItem->data(JidRole).toString());
    item.setNode(wdgItem->data(NodeRole).toString());
    m_trail.append(item);
    m_backButton->setEnabled(true);

    explore(item);
}

void Discovery::slotShow()
{
    QXmppDiscoveryIq::Item item;
    item.setJid(m_client->configuration().domain());
    m_trail.clear();
    m_trail.append(item);
    explore(item);
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
