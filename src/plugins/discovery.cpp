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
#include <QShortcut>

#include "QXmppDiscoveryIq.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "discovery.h"

Discovery::Discovery(QXmppClient *client, QWidget *parent)
    : ChatPanel(parent),
    m_client(client)
{
    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addItem(headerLayout());

    setLayout(layout);
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Service discovery"));

    /* connect signals */
    connect(this, SIGNAL(showPanel()), this, SLOT(slotShow()));
    connect(this, SIGNAL(hidePanel()), this, SIGNAL(unregisterPanel()));
}

void Discovery::slotShow()
{
    QXmppDiscoveryIq iq;
    iq.setQueryType(QXmppDiscoveryIq::InfoQuery);
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
