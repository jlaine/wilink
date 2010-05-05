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

#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppArchiveManager.h"
#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppUtils.h"

#include "chat_dialog.h"
#include "chat_history.h"
#include "chat_roster.h"

ChatDialog::ChatDialog(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatConversation(jid, parent), client(xmppClient), joined(false), rosterModel(chatRosterModel)
{
    connect(this, SIGNAL(localStateChanged(QXmppMessage::State)), this, SLOT(chatStateChanged(QXmppMessage::State)));
    connect(client, SIGNAL(connected()), this, SLOT(join()));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(&client->getArchiveManager(), SIGNAL(archiveChatReceived(const QXmppArchiveChat &)), this, SLOT(archiveChatReceived(const QXmppArchiveChat &)));
    connect(&client->getArchiveManager(), SIGNAL(archiveListReceived(const QList<QXmppArchiveChat> &)), this, SLOT(archiveListReceived(const QList<QXmppArchiveChat> &)));
    connect(this, SIGNAL(showPanel()), this, SLOT(join()));
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
        message.from = msg.isReceived() ? chatRemoteName : chatLocalName;
        message.received = msg.isReceived();
        chatHistory->addMessage(message);
    }
}

void ChatDialog::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with()) == chatRemoteJid)
            client->getArchiveManager().retrieveCollection(chats[i].with(), chats[i].start());
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
    client->getArchiveManager().listCollections(chatRemoteJid,
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
    message.date = QDateTime::currentDateTime();
    foreach (const QXmppElement &extension, msg.extensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_delay)
        {
            const QString str = extension.attribute("stamp");
            message.date = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
            message.date.setTimeSpec(Qt::UTC);
        }
    }
    message.from = chatRemoteName;
    message.received = true;
    chatHistory->addMessage(message);

    // notify
    emit notifyPanel();
}

void ChatDialog::sendMessage(const QString &text)
{
    // add message to history
    ChatHistoryMessage message;
    message.body = text;
    message.date = QDateTime::currentDateTime();
    message.from = chatLocalName;
    message.received = false;
    chatHistory->addMessage(message);

    // send message
    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(chatRemoteJid);
    msg.setState(QXmppMessage::Active);
    client->sendPacket(msg);
}

