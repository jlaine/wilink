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

#include <QDebug>
#include <QShortcut>

#include "chat.h"
#include "chat_console.h"
#include "chat_plugin.h"

class DiagnosticsPlugin : public ChatPlugin
{
public:
    void registerPlugin(Chat *chat);
};

void DiagnosticsPlugin::registerPlugin(Chat *chat)
{
    qDebug() << "register chat";

    ChatConsole *chatConsole = new ChatConsole(chat->chatClient()->logger(), chat);
    chatConsole->setObjectName("console");
    connect(chatConsole, SIGNAL(closeTab()), chat, SLOT(closePanel()));
    connect(chatConsole, SIGNAL(showTab()), chat, SLOT(showPanel()));

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_D), chat);
    connect(shortcut, SIGNAL(activated()), chatConsole, SIGNAL(showTab()));
}

Q_EXPORT_STATIC_PLUGIN2(diagnostics, DiagnosticsPlugin)
