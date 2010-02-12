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

#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"

#include "chat_edit.h"
#include "chat_history.h"
#include "chat_room.h"

ChatRoom::ChatRoom(const QString &jid, QWidget *parent)
    : ChatDialog(jid, parent)
{
}

bool ChatRoom::isRoom() const
{
    return true;
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QStringList bits = msg.getFrom().split("/");
    if (bits.size() != 2)
        return;
    const QString from = bits[1];

    ChatHistoryMessage message;
    message.body = msg.getBody();
    message.from = from;
    message.local = (from == chatLocalName);
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);
}

void ChatRoom::sendMessage(const QString &text)
{
    QXmppMessage msg;
    msg.setBody(text);
    msg.setFrom(chatRemoteJid + "/" + chatLocalName);
    msg.setTo(chatRemoteJid);
    msg.setType(QXmppMessage::GroupChat);
    emit sendPacket(msg);
}

ChatRoomPrompt::ChatRoomPrompt(QXmppClient *client, const QString &roomServer, QWidget *parent)
    : QDialog(parent), chatRoomServer(roomServer)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Enter the name of the chat room you want to join.")));
    lineEdit = new QLineEdit;
    layout->addWidget(lineEdit);
    listWidget = new QListWidget;
    listWidget->hide();
    listWidget->setSortingEnabled(true);
    layout->addWidget(listWidget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    setLayout(layout);

    setWindowTitle(tr("Join a chat room"));
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));

    // get rooms
    QXmppDiscoveryIq disco;
    disco.setTo(chatRoomServer);
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    client->sendPacket(disco);
}

void ChatRoomPrompt::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.getQueryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.getFrom() == chatRoomServer)
    {
        // chat rooms list
        listWidget->clear();
        foreach (const QXmppElement &item, disco.getItems())
            listWidget->addItem(item.attribute("jid").split("@").first());
        listWidget->show();
    }
}

QString ChatRoomPrompt::textValue() const
{
    return lineEdit->text();
}

void ChatRoomPrompt::validate()
{
    QString jid = lineEdit->text().trimmed().replace(" ", "_").toLower();
    lineEdit->setText(jid);
    if (jid.isEmpty())
        return;
    if (!jid.contains("@"))
        lineEdit->setText(jid + "@" + chatRoomServer);
    accept();
}
