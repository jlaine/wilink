/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <QContextMenuEvent>
#include <QDebug>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextEdit>

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"

#include "qnetio/dns.h"
#include "chat.h"

using namespace QNetIO;

ContactsList::ContactsList(QWidget *parent)
    : QListWidget(parent), showOffline(true)
{
    contextMenu = new QMenu(this);
    QAction *action = contextMenu->addAction(tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotItemDoubleClicked(QListWidgetItem*)));
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setMinimumSize(QSize(140, 140));
}

void ContactsList::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

    contextMenu->popup(event->globalPos());
}

void ContactsList::addEntry(const QXmppRoster::QXmppRosterEntry &entry)
{
    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setIcon(QIcon(":/contact-offline.png"));
    QString jid = entry.getBareJid();
    newItem->setData(Qt::UserRole, jid);
    QString name = entry.getName();
    if (name.isEmpty())
        name = jid.split("@")[0];
    newItem->setText(name);
    addItem(newItem);
    if (!showOffline)
        setItemHidden(newItem, true);
}

void ContactsList::removeContact()
{
    QListWidgetItem *item = currentItem();
    if (!item)
        return;

    QString jid = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        emit removeContact(jid);
    }
}

void ContactsList::presenceReceived(const QXmppPresence &presence)
{
    QString suffix;
    bool offline;
    switch (presence.getType())
    {
    case QXmppPresence::Available:
        switch(presence.getStatus().getType())
        {
            case QXmppPresence::Status::Online:
                suffix = "available";
                break;
            default:
                suffix = "busy";
                break;
        }
        offline = false;
        break;
    case QXmppPresence::Unavailable:
        suffix = "offline";
        offline = true;
        break;
    default:
        return;
    }
    QString bareJid = presence.getFrom().split("/").first();
    for (int i = 0; i < count(); i++)
    {
        QListWidgetItem *entry = item(i);
        if (entry->data(Qt::UserRole).toString() == bareJid)
        {
            entry->setIcon(QIcon(QString(":/contact-%1.png").arg(suffix)));
            bool hidden = !showOffline && offline;
            setItemHidden(entry, hidden);
            break;
        }
    }
}

void ContactsList::setShowOffline(bool show)
{
    // FIXME: refresh list
    showOffline = show;
}

void ContactsList::slotItemDoubleClicked(QListWidgetItem *item)
{
    emit chatContact(item->data(Qt::UserRole).toString());
}

ChatDialog::ChatDialog(QWidget *parent, const QString &jid)
    : QDialog(parent), chatJid(jid)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    chatHistory = new QTextEdit;
    chatHistory->setReadOnly(true);
    layout->addWidget(chatHistory);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);
    chatInput = new QLineEdit;
    hbox->addWidget(chatInput);

    QPushButton *sendButton = new QPushButton(tr("Send"));
    connect(sendButton, SIGNAL(clicked()), this, SLOT(send()));
    hbox->addWidget(sendButton);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowTitle(tr("Chat with %1").arg(jid));
    chatInput->setFocus();
}

void ChatDialog::handleMessage(const QXmppMessage &msg)
{
    chatHistory->append("<b>Received</b> " + msg.getBody());
}

void ChatDialog::send()
{
    QString text = chatInput->text();
    if (text.isEmpty())
        return;

    chatHistory->append("<b>Sent</b> " + text);
    chatInput->clear();
    emit sendMessage(chatJid, text);
}

Chat::Chat(QSystemTrayIcon *trayIcon)
    : systemTrayIcon(trayIcon)
{
    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    contacts = new ContactsList;
    connect(contacts, SIGNAL(chatContact(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(contacts, SIGNAL(removeContact(const QString&)), this, SLOT(removeContact(const QString&)));
    layout->addWidget(contacts);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);

    QPushButton *addButton = new QPushButton("+");
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    hbox->addWidget(addButton);
    hbox->addStretch();

    statusLabel = new QLabel;
    hbox->addWidget(statusLabel);
    layout->addItem(hbox);

    setLayout(layout);

    /* set up client */
    client = new QXmppClient(this);
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(handleMessage(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(handlePresence(const QXmppPresence&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), contacts, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(&client->getRoster(), SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(&client->getRoster(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
}

void Chat::addContact()
{
    bool ok = false;
    QString jid = QInputDialog::getText(this, "Add contact", "Enter the ID of the contact you want to add",
        QLineEdit::Normal, "@wifirst.net", &ok);
    if (!ok || jid.isEmpty())
        return;

    QXmppPresence packet;
    packet.setTo(jid);
    packet.setType(QXmppPresence::Subscribe);
    client->sendPacket(packet);
}

ChatDialog *Chat::chatContact(const QString &jid)
{
    if (!chatDialogs.contains(jid))
    {
        chatDialogs[jid] = new ChatDialog(this, jid);
        connect(chatDialogs[jid], SIGNAL(sendMessage(const QString&, const QString&)),
            client, SLOT(sendMessage(const QString&, const QString &)));
    }
    chatDialogs[jid]->show();
    return chatDialogs[jid];
}

void Chat::connected()
{
    statusLabel->setText(tr("Connected"));
}

void Chat::disconnected()
{
    statusLabel->setText(tr("Disconnected"));
}

void Chat::handleMessage(const QXmppMessage &msg)
{
    const QString jid = msg.getFrom();
    const QString body = msg.getBody();
    const QString from = jid.split("@")[0];

    if (body.isEmpty())
        return;

    ChatDialog *dialog = chatContact(jid.split("/")[0]);
    dialog->handleMessage(msg);
  
/* 
    if (systemTrayIcon)
        systemTrayIcon->showMessage(from, body);
*/
}

void Chat::handlePresence(const QXmppPresence &presence)
{
    QXmppPresence packet;
    packet.setTo(presence.getFrom());
    switch (presence.getType())
    {
    case QXmppPresence::Subscribe:
        {
            if (QMessageBox::question(this,
                tr("Invitation from %1").arg(presence.getFrom()),
                tr("%1 has asked to add you to his or her contact list. Do you accept?").arg(presence.getFrom()),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                qDebug("Subscribe accepted");
                packet.setType(QXmppPresence::Subscribed);
                client->sendPacket(packet);

                packet.setType(QXmppPresence::Subscribe);
                client->sendPacket(packet);
            } else {
                qDebug("Subscribe refused");
                QXmppPresence packet;
                packet.setType(QXmppPresence::Unsubscribed);
                client->sendPacket(packet);
            }
        }
        break;
    default:
        break;
    }
}

bool Chat::open(const QString &jid, const QString &password)
{
    QXmppConfiguration config;
    config.setResource("wDesktop");

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::NONE);

    /* get user and domain */
    QStringList bits = jid.split("@");
    if (bits.size() != 2)
    {
        qWarning("Invalid JID");
        return false;
    }
    config.setUser(bits[0]);
    config.setDomain(bits[1]);

    /* get the server */
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.getDomain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.getDomain());
    }

    /* connect to server */
    statusLabel->setText(tr("Connecting.."));
    config.setPasswd(password);
    client->connectToServer(config);
    return true;
}

void Chat::removeContact(const QString &jid)
{
    qDebug() << "Sending unsubscribe to" << jid;
    QXmppRosterIq::Item item;
    item.setBareJid(jid);
    item.setSubscriptionType(QXmppRosterIq::Item::Remove);
    QXmppRosterIq packet(QXmppIq::Set);
    packet.addItem(item);
/*
    QXmppPresence packet;
    packet.setTo(jid);
    packet.setType(QXmppPresence::Unsubscribe);
*/
    client->sendPacket(packet);
}

void Chat::rosterChanged(const QString &jid)
{
    QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(jid);
    int itemIndex = -1;
    for (int i = 0; i < contacts->count(); i++)
    {
        QListWidgetItem *item = contacts->item(i);
        if (item->data(Qt::UserRole).toString() == jid)
        {
            itemIndex = i;
            break;
        }
    }
    switch (entry.getSubscriptionType())
    {
        case QXmppRosterIq::Item::Remove:
            if (itemIndex >= 0)
                contacts->takeItem(itemIndex);
            break;
        default:
            if (itemIndex < 0)
                contacts->addEntry(entry);
    }
}

void Chat::rosterReceived()
{
    QMap<QString, QXmppRoster::QXmppRosterEntry> entries = client->getRoster().getRosterEntries();
    contacts->clear();
    foreach (const QString &key, entries.keys())
        contacts->addEntry(entries[key]);
}

