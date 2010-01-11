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
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextBrowser>

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "qnetio/dns.h"
#include "chat.h"
#include "chat_dialog.h"
#include "chat_roster.h"

using namespace QNetIO;

Chat::Chat(QSystemTrayIcon *trayIcon)
    : systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    contacts = new RosterView(*client);
    connect(contacts, SIGNAL(chatContact(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(contacts, SIGNAL(removeContact(const QString&)), this, SLOT(removeContact(const QString&)));
    connect(contacts->model(), SIGNAL(modelReset()), this, SLOT(resizeContacts()));
    layout->addWidget(contacts);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    hbox->addWidget(addButton);
    hbox->addStretch();

    statusLabel = new QLabel;
    hbox->addWidget(statusLabel);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowIcon(QIcon(":/chat.png"));
    setWindowTitle(tr("Chat"));

    /* set up client */
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));

    connect(&client->getVCardManager(), SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));
}

void Chat::addContact()
{
    bool ok = false;
    QString jid = QInputDialog::getText(this, tr("Add a contact"),
        tr("Enter the address of the contact you want to add."),
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
        QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(jid);
        QString name = entry.getName();
        if (name.isEmpty())
            name = jid.split("@")[0];

        chatDialogs[jid] = new ChatDialog(jid, name);
        connect(chatDialogs[jid], SIGNAL(sendMessage(const QString&, const QString&)),
            this, SLOT(sendMessage(const QString&, const QString&)));
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

void Chat::messageReceived(const QXmppMessage &msg)
{
    const QString jid = msg.getFrom();
    const QString body = msg.getBody();
    const QString from = jid.split("@")[0];

    if (body.isEmpty())
        return;

    ChatDialog *dialog = chatContact(jid.split("/")[0]);
    dialog->messageReceived(msg);
}

void Chat::presenceReceived(const QXmppPresence &presence)
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
    if (QMessageBox::question(this, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        qDebug() << "Sending unsubscribe to" << jid;
        QXmppRosterIq::Item item;
        item.setBareJid(jid);
        item.setSubscriptionType(QXmppRosterIq::Item::Remove);
        QXmppRosterIq packet(QXmppIq::Set);
        packet.addItem(item);
        client->sendPacket(packet);
    }
}

void Chat::resizeContacts()
{
    QSize hint = contacts->sizeHint();
    hint.setHeight(hint.height() + 32);
    resize(hint);
}

void Chat::sendMessage(const QString &jid, const QString message)
{
    client->sendPacket(QXmppMessage("", jid, message));
}

void Chat::vCardReceived(const QXmppVCard& vcard)
{
    const QString bareJid = vcard.getFrom().split("/").first();
    if (!chatDialogs.contains(bareJid))
        return;
}

