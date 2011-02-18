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

#ifndef __WILINK_CHAT_PLUGIN_H__
#define __WILINK_CHAT_PLUGIN_H__

#include <QtPlugin>

class Chat;
class ChatPanel;
class ChatPreferences;

/** Interface for all plugins.
 */
class ChatPluginInterface
{
public:
    virtual bool initialize(Chat *chat) = 0;
    virtual void finalize(Chat *) {};
    virtual QString name() const = 0;
    virtual void preferences(ChatPreferences *prefs) {};
};

Q_DECLARE_INTERFACE(ChatPluginInterface, "net.wifirst.ChatPlugin/1.0")

class ChatPlugin : public QObject, public ChatPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(ChatPluginInterface)
};

#endif
