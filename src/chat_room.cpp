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

ChatRoomOptions::ChatRoomOptions(QXmppClient *xmppClient, const QString &roomJid, QWidget *parent)
    : QDialog(parent), chatRoomJid(roomJid), client(xmppClient)
{
    QVBoxLayout *layout = new QVBoxLayout;

    frame = new QFrame;
    layout->addWidget(frame);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    setLayout(layout);

    setWindowTitle(tr("Chat room options"));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));

    // get room info
    QXmppIq iq;
    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_owner);
    iq.setItems(query);
    iq.setTo(chatRoomJid);
    client->sendPacket(iq);
}

void ChatRoomOptions::iqReceived(const QXmppIq &iq)
{
    if (iq.getType() != QXmppIq::Result ||
        iq.getFrom() != chatRoomJid)
        return;
    const QXmppElement query = iq.getItems().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_owner)
        return;
    form = query.firstChild("x");
    if (form.attribute("type") != "form" || form.attribute("xmlns") != "jabber:x:data")
        return;

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(0);
    foreach (const QXmppElement &field, form.children())
    {
        if (field.tagName() == "field" && !field.attribute("var").isEmpty())
        {
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = new QCheckBox(field.attribute("label"));
                checkbox->setChecked(field.firstChild("value").value() == "1");
                checkbox->setObjectName(field.attribute("var"));
                vbox->addWidget(checkbox);
            } else if (field.attribute("type") == "text-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(field.attribute("label")));
                QLineEdit *edit = new QLineEdit(field.firstChild("value").value());
                edit->setObjectName(field.attribute("var"));
                hbox->addWidget(edit);
                vbox->addItem(hbox);
            } else if (field.attribute("type") == "list-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(field.attribute("label")));
                QComboBox *combo = new QComboBox;
                combo->setObjectName(field.attribute("var"));
                int currentIndex = 0;
                const QString currentValue = field.firstChild("value").value();
                int index = 0;
                foreach (const QXmppElement &option, field.children())
                {
                    if (option.tagName() == "option")
                    {
                        const QString value = option.firstChild("value").value();
                        combo->addItem(option.attribute("label"), value);
                        if (value == currentValue)
                            currentIndex = index;
                        index++;
                    }
                }
                combo->setCurrentIndex(currentIndex);
                hbox->addWidget(combo);
                vbox->addItem(hbox);
            }
        }
    }
    frame->setLayout(vbox);
}

void ChatRoomOptions::submit()
{
    form.setAttribute("type", "submit");
    QXmppElementList formFields;
    foreach (QXmppElement field, form.children())
    {
        if (field.tagName() == "field" && !field.attribute("var").isEmpty())
        {
            QXmppElement value = field.firstChild("value");
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = frame->findChild<QCheckBox*>(field.attribute("var"));
                value.setValue(checkbox->checkState() == Qt::Checked ? "1" : "0");
                field.setChildren(value);
            } else if (field.attribute("type") == "text-single") {
                QLineEdit *edit = frame->findChild<QLineEdit*>(field.attribute("var"));
                value.setValue(edit->text());
                field.setChildren(value);
            }
        }
        formFields.append(field);
    }
    form.setChildren(formFields);

    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_owner);
    query.setChildren(form);

    QXmppIq iq(QXmppIq::Set);
    iq.setItems(query);
    iq.setTo(chatRoomJid);
    client->sendPacket(iq);

    accept();
}
