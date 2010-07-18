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

#include <QDesktopServices>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStatusBar>
#include <QUrl>

#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "utils.h"

#include "contacts.h"

ContactsWatcher::ContactsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    ChatClient *client = chat->client();
    connect(client, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
            this, SLOT(presenceReceived(const QXmppPresence&)));

    // add button to status bar
    addButton = new QPushButton;
    addButton->setEnabled(false);
    addButton->setIcon(QIcon(":/add.png"));
    addButton->setToolTip(tr("Add a contact"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    chat->statusBar()->insertWidget(0, addButton);
}

/** Prompt the user for a new contact then add it to the roster.
 */
void ContactsWatcher::addContact()
{
    bool ok = true;
    QString jid = "@" + chat->client()->getConfiguration().domain();
    while (!isBareJid(jid))
    {
        jid = QInputDialog::getText(chat, tr("Add a contact"),
            tr("Enter the address of the contact you want to add."),
            QLineEdit::Normal, jid, &ok).toLower();
        if (!ok)
            return;
        jid = jid.trimmed().toLower();
    }

    QXmppPresence packet;
    packet.setTo(jid);
    packet.setType(QXmppPresence::Subscribe);
    chat->client()->sendPacket(packet);
}

void ContactsWatcher::connected()
{
    addButton->setEnabled(true);
}

void ContactsWatcher::disconnected()
{
    addButton->setEnabled(false);
}

/** When the user has decided whether or not to accept a subscription request,
 *  update the roster accordingly.
 */
void ContactsWatcher::presenceHandled(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    if (!box)
        return;
    
    QString jid = box->objectName();
    ChatClient *client = chat->client();
            
    QXmppPresence packet;
    packet.setTo(jid);
    if (box->standardButton(button) == QMessageBox::Yes)
    {
        // accept subscription
        packet.setType(QXmppPresence::Subscribed);
        client->sendPacket(packet);

        // request reciprocal subscription
        packet.setType(QXmppPresence::Subscribe);
        client->sendPacket(packet);
    } else {
        // refuse subscription
        packet.setType(QXmppPresence::Unsubscribed);
        client->sendPacket(packet);
    }
    box->deleteLater();
}

/** Prompt the user to accept or refuse a subscription request.
 */
void ContactsWatcher::presenceReceived(const QXmppPresence &presence)
{
    const QString bareJid = jidToBareJid(presence.from());

    if (presence.type() == QXmppPresence::Subscribe)
    {
        ChatClient *client = chat->client();
        const QString jid = presence.from();
        QXmppRoster::QXmppRosterEntry entry = client->rosterManager().getRosterEntry(jid);
        QXmppRoster::QXmppRosterEntry::SubscriptionType type = entry.subscriptionType();

        /* if the contact is in our roster accept subscribe */
        if (type == QXmppRoster::QXmppRosterEntry::To || type == QXmppRoster::QXmppRosterEntry::Both)
        {
            // accept subscription
            QXmppPresence packet;
            packet.setTo(jid);
            packet.setType(QXmppPresence::Subscribed);
            client->sendPacket(packet);

            // request reciprocal subscription
            packet.setType(QXmppPresence::Subscribe);
            client->sendPacket(packet);
            return;
        }

        /* otherwise ask user */
        QMessageBox *box = new QMessageBox(QMessageBox::Question,
            tr("Invitation from %1").arg(jid),
            tr("%1 has asked to add you to his or her contact list.\n\nDo you accept?").arg(jid),
            QMessageBox::Yes | QMessageBox::No,
            chat);
        box->setObjectName(jid);
        box->setDefaultButton(QMessageBox::Yes);
        box->setEscapeButton(QMessageBox::No);
        box->open(this, SLOT(presenceHandled(QAbstractButton*)));
    }
}

/** Prompt the user for confirmation then remove a contact.
 */
void ContactsWatcher::removeContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    if (QMessageBox::question(chat, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        QXmppRosterIq::Item item;
        item.setBareJid(jid);
        item.setSubscriptionType(QXmppRosterIq::Item::Remove);
        QXmppRosterIq packet;
        packet.setType(QXmppIq::Set);
        packet.addItem(item);
        chat->client()->sendPacket(packet);
    }
}

/** Prompt the user to rename a contact.
 */
void ContactsWatcher::renameContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    bool ok = true;
    QXmppRosterIq::Item item = chat->client()->rosterManager().getRosterEntry(jid);
    QString name = QInputDialog::getText(chat, tr("Rename contact"),
        tr("Enter the name for this contact."),
        QLineEdit::Normal, item.name(), &ok);
    if (ok)
    {
        item.setName(name);
        QXmppRosterIq packet;
        packet.setType(QXmppIq::Set);
        packet.addItem(item);
        chat->client()->sendPacket(packet);
    }
}

void ContactsWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString bareJid = index.data(ChatRosterModel::IdRole).toString();
    
    if (type == ChatRosterItem::Contact)
    {
        QAction *action;

        const QString url = index.data(ChatRosterModel::UrlRole).toString();
        if (!url.isEmpty())
        {
            action = menu->addAction(QIcon(":/diagnostics.png"), tr("Show profile"));
            action->setData(url);
            connect(action, SIGNAL(triggered()), this, SLOT(showContactPage()));
        }

        action = menu->addAction(QIcon(":/options.png"), tr("Rename contact"));
        action->setData(bareJid);
        connect(action, SIGNAL(triggered()), this, SLOT(renameContact()));

        action = menu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
        action->setData(bareJid);
        connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));
    }
}

/** Show a contact's web page.
 */
void ContactsWatcher::showContactPage()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    QString url = action->data().toString();
    if (!url.isEmpty())
        QDesktopServices::openUrl(url);
}

// PLUGIN

class ContactsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool ContactsPlugin::initialize(Chat *chat)
{
    ContactsWatcher *contacts = new ContactsWatcher(chat);
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            contacts, SLOT(rosterMenu(QMenu*, QModelIndex)));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(contacts, ContactsPlugin)

