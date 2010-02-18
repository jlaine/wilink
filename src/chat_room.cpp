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

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"

#include "chat.h"
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
    if (msg.getExtension().attribute("xmlns") == ns_delay)
    {
        const QString str = msg.getExtension().attribute("stamp");
        message.datetime = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
        message.datetime.setTimeSpec(Qt::UTC);
    } else {
        message.datetime = QDateTime::currentDateTime();
    }
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
    listWidget->setIconSize(QSize(32, 32));
    listWidget->setSortingEnabled(true);
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
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
    if (disco.getType() == QXmppIq::Result &&
        disco.getQueryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.getFrom() == chatRoomServer)
    {
        // chat rooms list
        listWidget->clear();
        foreach (const QXmppElement &item, disco.getQueryItems())
        {
            QString jid = item.attribute("jid");
            QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), item.attribute("name"));
            wdgItem->setData(Qt::UserRole, jid);
            listWidget->addItem(wdgItem);
        }
    }
}

void ChatRoomPrompt::itemClicked(QListWidgetItem *item)
{
    lineEdit->setText(item->data(Qt::UserRole).toString());
    validate();
}

QString ChatRoomPrompt::textValue() const
{
    return lineEdit->text();
}

void ChatRoomPrompt::validate()
{
    QString jid = lineEdit->text();
    if (jid.contains(" ") || jid.isEmpty())
    {
        lineEdit->setText(jid.trimmed().replace(" ", "_"));
        return;
    }
    if (!jid.contains("@"))
        lineEdit->setText(jid.toLower() + "@" + chatRoomServer);
    else
        lineEdit->setText(jid.toLower());
    accept();
}

ChatRoomMembers::ChatRoomMembers(QXmppClient *xmppClient, const QString &roomJid, QWidget *parent)
    : QDialog(parent), chatRoomJid(roomJid), client(xmppClient)
{
    QVBoxLayout *layout = new QVBoxLayout;

    frame = new QFrame;
    layout->addWidget(frame);

    listWidget = new QListWidget(this);
    layout->addWidget(listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addMember()));
    buttonBox->addButton(addButton, QDialogButtonBox::ActionRole);

    QPushButton *removeButton = new QPushButton;
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);
    setLayout(layout);

    setWindowTitle(tr("Chatroom members"));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));

    QXmppElementList elements;

    QXmppElement item;
    item.setTagName("item");
    item.setAttribute("affiliation", "member");
    elements.append(item);
    item.setAttribute("affiliation", "outcast");
    elements.append(item);
    item.setAttribute("affiliation", "owner");
    elements.append(item);
    item.setAttribute("affiliation", "admin");
    elements.append(item);
    item.setAttribute("affiliation", "none");
    elements.append(item);

    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_admin);
    query.setChildren(elements);

    QXmppIq iq;
    iq.setItems(query);
    iq.setTo(chatRoomJid);
    client->sendPacket(iq);
}

void ChatRoomMembers::iqReceived(const QXmppIq &iq)
{
    if (iq.getType() != QXmppIq::Result ||
        iq.getFrom() != chatRoomJid)
        return;
    const QXmppElement query = iq.getItems().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_admin)
        return;

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(0);
    foreach(QXmppElement element, query.children())
    {
        listWidget->addItem(element.attribute("jid")); // FIXME: show a 2nd columns (one for the affiliation)
    }
    frame->setLayout(vbox);
}

void ChatRoomMembers::submit()
{
    qDebug() << "Submitting!";
}

void ChatRoomMembers::addMember()
{
    qDebug() << "Adding a member!";
}

void ChatRoomMembers::removeMember()
{
    qDebug() << "Removing a member!";
}
