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

#include "chat.h"
#include "chat_plugin.h"
#include "player.h"

PlayerPanel::PlayerPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow),
    m_playId(-1)
{
    setObjectName("z_2_player");
    setWindowIcon(QIcon(":/start.png"));
    setWindowTitle(tr("Media player"));

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(10);

    setLayout(layout);

    // register panel
    QMetaObject::invokeMethod(this, "registerPanel", Qt::QueuedConnection);
}

void PlayerPanel::doubleClicked(const QModelIndex &index)
{

}

// PLUGIN

class PlayerPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool PlayerPlugin::initialize(Chat *chat)
{
    PlayerPanel *panel = new PlayerPanel(chat);
    chat->addPanel(panel);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(player, PlayerPlugin)

