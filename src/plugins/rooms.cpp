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

#include <QMenu>

#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_room.h"

#include "rooms.h"

ChatRoomWatcher::ChatRoomWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    connect(chat->chatClient(), SIGNAL(messageReceived(QXmppMessage)),
            this, SLOT(messageReceived(QXmppMessage)));
}

/** Invite a contact to join a chat room.
 */
void ChatRoomWatcher::inviteContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    ChatRoomPrompt prompt(chat->chatClient(), chatRoomServer, chat);
    if (!prompt.exec())
        return;

    // join chat room
    const QString roomJid = prompt.textValue();
    chat->rosterAction(ChatRosterView::JoinAction, roomJid, ChatRosterItem::Room);

    // invite contact
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_conference);
    x.setAttribute("jid", roomJid);
    x.setAttribute("reason", "Let's talk");

    QXmppMessage message;
    message.setTo(jid);
    message.setType(QXmppMessage::Normal);
    message.setExtensions(x);
    chat->chatClient()->sendPacket(message);
}

void ChatRoomWatcher::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.from());

    if (msg.type() == QXmppMessage::Normal)
    {
        foreach (const QXmppElement &extension, msg.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
            {
                const QString contactName = chat->rosterModel()->contactName(bareJid);
                const QString roomJid = extension.attribute("jid");
                if (!roomJid.isEmpty() && !chat->panel(roomJid))
                {
                    ChatRoomInvitePrompt *dlg = new ChatRoomInvitePrompt(contactName, roomJid, chat);
                    connect(dlg, SIGNAL(itemAction(int, const QString&, int)), chat, SLOT(rosterAction(int, const QString&, int)));
                    dlg->show();
                }
                break;
            }
        }
    }
}


void ChatRoomWatcher::rosterMenu(QMenu *menu, const QString &jid, int type)
{
    if (type == ChatRosterItem::Contact)
    {
        QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Invite to a chat room"));
        action->setData(jid);
        connect(action, SIGNAL(triggered()), this, SLOT(inviteContact()));
    }
}

// PLUGIN

class RoomsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool RoomsPlugin::initialize(Chat *chat)
{
    ChatRoomWatcher *rooms = new ChatRoomWatcher(chat);
    connect(chat, SIGNAL(rosterMenu(QMenu*, QString, int)), rooms, SLOT(rosterMenu(QMenu*, QString, int)));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(rooms, RoomsPlugin)

