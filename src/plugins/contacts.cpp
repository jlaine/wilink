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
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStatusBar>
#include <QUrl>

#include "QXmppClient.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_panel.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_utils.h"

#include "contacts.h"

class ContactsPanel : public ChatPanel
{
public:
    ContactsPanel(Chat *chatWindow, const QString &jid, QLabel *tip);
};

ContactsPanel::ContactsPanel(Chat *chatWindow, const QString &jid, QLabel *tip)
    : ChatPanel(chatWindow)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addWidget(tip, 1);
    setLayout(layout);

    bool check;
    check = connect(this, SIGNAL(hidePanel()),
                    this, SLOT(deleteLater()));
    Q_ASSERT(check);

    setObjectName(jid);
    setWindowTitle(chatWindow->rosterModel()->contactName(jid));
    setWindowIcon(chatWindow->rosterModel()->contactAvatar(jid));
}

ContactsWatcher::ContactsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    QXmppClient *client = chat->client();
    connect(client, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
            this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(chat, SIGNAL(rosterClick(QModelIndex)),
            this, SLOT(rosterClick(QModelIndex)));

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
    const QString domain = chat->client()->configuration().domain();
    QString jid = QLatin1String("@") + domain;

    QDialog dialog(chat);
    QVBoxLayout *vbox = new QVBoxLayout;

    // tip
    if (domain == "wifirst.net")
        vbox->addWidget(tipLabel());

    // prompt label
    QLabel *label = new QLabel(tr("Enter the address of the contact you want to add."));
    vbox->addWidget(label);

    // input box
    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setText(jid);
    lineEdit->setCursorPosition(0);
    vbox->addWidget(lineEdit);

    // buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    vbox->addWidget(buttonBox);

    dialog.setLayout(vbox);
    dialog.setWindowIcon(QIcon(":/add.png"));
    dialog.setWindowTitle(tr("Add a contact"));

    while (!isBareJid(jid))
    {
        // show prompt
        if (dialog.exec() != QDialog::Accepted)
            return;
        jid = lineEdit->text().trimmed().toLower();
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
    QXmppClient *client = chat->client();
            
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
        QXmppClient *client = chat->client();
        const QString jid = presence.from();
        QXmppRosterIq::Item entry = client->rosterManager().getRosterEntry(jid);
        QXmppRosterIq::Item::SubscriptionType type = entry.subscriptionType();

        /* if the contact is in our roster accept subscribe */
        if (type == QXmppRosterIq::Item::To || type == QXmppRosterIq::Item::Both)
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
    const QString bareJid = action->data().toString();
    const QString contactName = chat->rosterModel()->contactName(bareJid);

    if (QMessageBox::question(chat, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(contactName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        QXmppRosterIq::Item item;
        item.setBareJid(bareJid);
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

/** When the user clicks on a contact in his roster, open a conversation.
 *
 * @param index The roster entry that was clicked.
 */
void ContactsWatcher::rosterClick(const QModelIndex &index)
{
    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    const QString contactsJid = chat->rosterModel()->contactsItem().data(ChatRosterModel::IdRole).toString();
    const QString domain = chat->client()->configuration().domain();

    if (domain == "wifirst.net" && jid == contactsJid && !chat->panel(jid))
        chat->addPanel(new ContactsPanel(chat, jid, tipLabel()));
}

void ContactsWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString bareJid = index.data(ChatRosterModel::IdRole).toString();
    
    QAction *action;
    if (type == ChatRosterItem::Contact || type == ChatRosterItem::RoomMember)
    {
        const QString url = index.data(ChatRosterModel::UrlRole).toString();
        if (!url.isEmpty())
        {
            action = menu->addAction(QIcon(":/diagnostics.png"), tr("Show profile"));
            action->setData(url);
            connect(action, SIGNAL(triggered()), this, SLOT(showContactPage()));
        }
    }

    if (type == ChatRosterItem::Contact)
    {
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

QLabel *ContactsWatcher::tipLabel() const
{
    QLabel *tip = new QLabel(tr("<b>Tip</b>: your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>!").arg("https://www.wifirst.net/w/friends?from=wiLink"));
    tip->setOpenExternalLinks(true);
    tip->setWordWrap(true);
    return tip;
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

