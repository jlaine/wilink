/*
 * wDesktop
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
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_dialog.h"
#include "chat_edit.h"
#include "chat_history.h"

ChatDialog::ChatDialog(QXmppClient *xmppClient, const QString &jid, QWidget *parent)
    : ChatConversation(jid, parent), client(xmppClient)
{
    connect(this, SIGNAL(stateChanged(QXmppMessage::State)), this, SLOT(chatStateChanged(QXmppMessage::State)));
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(&client->getArchiveManager(), SIGNAL(archiveChatReceived(const QXmppArchiveChat &)), this, SLOT(archiveChatReceived(const QXmppArchiveChat &)));
    connect(&client->getArchiveManager(), SIGNAL(archiveListReceived(const QList<QXmppArchiveChat> &)), this, SLOT(archiveListReceived(const QList<QXmppArchiveChat> &)));
}

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
{
    if (jidToBareJid(chat.with) != chatRemoteJid)
        return;

    foreach (const QXmppArchiveMessage &msg, chat.messages)
    {
        ChatHistoryMessage message;
        message.archived = true;
        message.body = msg.body;
        message.datetime = msg.datetime;
        message.from = msg.local ? chatLocalName : chatRemoteName;
        message.local = msg.local;
        chatHistory->addMessage(message);
    }
}

void ChatDialog::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with) == chatRemoteJid)
            client->getArchiveManager().retrieveCollection(chats[i].with, chats[i].start);
}

/** When the chat state changes, notify the remote party.
 */
void ChatDialog::chatStateChanged(QXmppMessage::State state)
{
    QXmppMessage message;
    message.setTo(chatRemoteJid);
    message.setState(state);
    client->sendPacket(message);
}

void ChatDialog::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results from remote party
    if (jidToBareJid(disco.getFrom()) != chatRemoteJid ||
        disco.getType() != QXmppIq::Result )
        return;

    qDebug("Received discovery result from remote party");
    foreach (const QXmppElement &element, disco.getQueryItems())
        dumpElement(element);
}

bool ChatDialog::isRoom() const
{
    return false;
}

/** Start a two party dialog.
 */
void ChatDialog::join()
{
#if 0
    // discover remote party features
    QXmppDiscoveryIq disco;
    disco.setTo(chatRemoteJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    client->sendPacket(disco);
#endif

    // list archives for the past week.
    client->getArchiveManager().listCollections(chatRemoteJid,
        QDateTime::currentDateTime().addDays(-7));
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    if (jidToBareJid(msg.getFrom()) != chatRemoteJid)
        return;

    setRemoteState(msg.getState());

    ChatHistoryMessage message;
    message.body = msg.getBody();
    message.datetime = QDateTime::currentDateTime();
    foreach (const QXmppElement &extension, msg.getExtensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_delay)
        {
            const QString str = extension.attribute("stamp");
            message.datetime = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
            message.datetime.setTimeSpec(Qt::UTC);
        }
    }
    message.from = chatRemoteName;
    message.local = false;
    chatHistory->addMessage(message);
}

void ChatDialog::sendMessage(const QString &text)
{
    // add message to history
    ChatHistoryMessage message;
    message.body = text;
    message.datetime = QDateTime::currentDateTime();
    message.from = chatLocalName;
    message.local = true;
    chatHistory->addMessage(message);

    // send message
    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(chatRemoteJid);
    msg.setState(QXmppMessage::Active);
    client->sendPacket(msg);
}

