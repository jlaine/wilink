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

#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>

#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppArchiveManager.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "chats.h"

ChatDialog::ChatDialog(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatConversation(parent),
    chatRemoteJid(jid), 
    client(xmppClient),
    joined(false),
    rosterModel(chatRosterModel)
{
    setObjectName(jid);
    setWindowTitle(rosterModel->contactName(jid));
    setWindowIcon(rosterModel->contactAvatar(jid));
    setWindowExtra(jid);

    connect(this, SIGNAL(localStateChanged(QXmppMessage::State)), this, SLOT(chatStateChanged(QXmppMessage::State)));
    connect(client, SIGNAL(connected()), this, SLOT(join()));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(&client->archiveManager(), SIGNAL(archiveChatReceived(const QXmppArchiveChat &)), this, SLOT(archiveChatReceived(const QXmppArchiveChat &)));
    connect(&client->archiveManager(), SIGNAL(archiveListReceived(const QList<QXmppArchiveChat> &)), this, SLOT(archiveListReceived(const QList<QXmppArchiveChat> &)));
    connect(this, SIGNAL(hidePanel()), this, SLOT(leave()));
    connect(this, SIGNAL(showPanel()), this, SLOT(join()));

    QTimer::singleShot(0, this, SIGNAL(registerPanel()));
}

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
{
    if (jidToBareJid(chat.with()) != chatRemoteJid)
        return;

    foreach (const QXmppArchiveMessage &msg, chat.messages())
    {
        ChatHistoryMessage message;
        message.archived = true;
        message.body = msg.body();
        message.date = msg.date();
        message.from = msg.isReceived() ? rosterModel->contactName(chatRemoteJid) : rosterModel->ownName();
        message.fromJid = msg.isReceived() ? chatRemoteJid : client->getConfiguration().jid();
        message.received = msg.isReceived();
        chatHistory->addMessage(message);
    }
}

void ChatDialog::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with()) == chatRemoteJid)
            client->archiveManager().retrieveCollection(chats[i].with(), chats[i].start());
}

/** When the chat state changes, notify the remote party.
 */
void ChatDialog::chatStateChanged(QXmppMessage::State state)
{
    foreach (const QString &jid, chatStatesJids)
    {
        QXmppMessage message;
        message.setTo(jid);
        message.setState(state);
        client->sendPacket(message);
    }
}

void ChatDialog::disconnected()
{
    // FIXME : we should re-join on connect
    // joined = false;
}

/** Leave a two party dialog.
 */
void ChatDialog::leave()
{
    if (joined)
    {
        chatStateChanged(QXmppMessage::Gone);
        joined = false;
    }
    deleteLater();
}

/** Start a two party dialog.
 */
void ChatDialog::join()
{
    if (joined)
        return;

    // send initial state
    chatStatesJids = rosterModel->contactFeaturing(chatRemoteJid, ChatRosterModel::ChatStatesFeature);
    foreach (const QString &fullJid, chatStatesJids)
    {
        QXmppMessage message;
        message.setTo(fullJid);
        message.setState(localState());
        client->sendPacket(message);
    }

    // list archives for the past week.
    // FIXME : we need to check whether archives are supported
    // to clear the display appropriately
    client->archiveManager().listCollections(chatRemoteJid,
        QDateTime::currentDateTime().addDays(-7));

    joined = true;
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    if (jidToBareJid(msg.from()) != chatRemoteJid)
        return;

    setRemoteState(msg.state());
    if (msg.body().isEmpty())
        return;

    ChatHistoryMessage message;
    message.body = msg.body();
    message.date = msg.stamp();
    if (!message.date.isValid())
        message.date = QDateTime::currentDateTime();
    message.from = rosterModel->contactName(chatRemoteJid);
    message.fromJid = chatRemoteJid;
    message.received = true;
    chatHistory->addMessage(message);

    // queue notification
    queueNotification(message.body);
}

/** Return the type of entry to add to the roster.
 */
ChatRosterItem::Type ChatDialog::objectType() const
{
    return ChatRosterItem::Contact;
}

bool ChatDialog::sendMessage(const QString &text)
{
    // send message
    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(chatRemoteJid);
    msg.setState(QXmppMessage::Active);
    if (!client->sendPacket(msg))
        return false;

    // add message to history
    ChatHistoryMessage message;
    message.body = text;
    message.date = QDateTime::currentDateTime();
    message.from = rosterModel->ownName();
    message.received = false;
    chatHistory->addMessage(message);

    return true;
}

ChatsWatcher::ChatsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    ChatClient *client = chat->client();
    connect(client, SIGNAL(messageReceived(QXmppMessage)),
            this, SLOT(messageReceived(QXmppMessage)));

    // add roster hooks
    connect(chat, SIGNAL(rosterClick(QModelIndex)),
            this, SLOT(rosterClick(QModelIndex)));
}

void ChatsWatcher::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.from());

    if (msg.type() == QXmppMessage::Chat && !chat->panel(bareJid) && !msg.body().isEmpty())
    {
        ChatDialog *dialog = new ChatDialog(chat->client(), chat->rosterModel(), bareJid);
        chat->addPanel(dialog);
        dialog->messageReceived(msg);
    }
}

void ChatsWatcher::rosterClick(const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // create conversation if necessary
    if (type == ChatRosterItem::Contact && !chat->panel(jid))
        chat->addPanel(new ChatDialog(chat->client(), chat->rosterModel(), jid));
}

// PLUGIN

class ChatsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool ChatsPlugin::initialize(Chat *chat)
{
    new ChatsWatcher(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(chats, ChatsPlugin)

