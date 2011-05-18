/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QMessageBox>

#include "QXmppClient.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "contacts.h"

ContactsWatcher::ContactsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    QXmppClient *client = chat->client();
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
            this, SLOT(presenceReceived(const QXmppPresence&)));

    // add contact panel
    ChatRosterProxyModel *contactModel = new ChatRosterProxyModel(this);
    contactModel->setSourceModel(chat->rosterModel());
    contactModel->setSourceRoot(chat->rosterModel()->contactsItem());

    QDeclarativeContext *context = chat->rosterView()->rootContext();
    context->setContextProperty("contactModel", contactModel);
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
            
    if (box->standardButton(button) == QMessageBox::Yes)
    {
        // accept subscription
        client->rosterManager().acceptSubscription(jid);

        // request reciprocal subscription
        client->rosterManager().subscribe(jid);
    } else {
        // refuse subscription
        client->rosterManager().refuseSubscription(jid);
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

        // if the contact is in our roster accept subscription
        if (type == QXmppRosterIq::Item::To || type == QXmppRosterIq::Item::Both)
        {
            // accept subscription
            client->rosterManager().acceptSubscription(jid);

            // request reciprocal subscription
            client->rosterManager().subscribe(jid);
            return;
        }

        // otherwise ask user
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

// PLUGIN

class ContactsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    void finalize(Chat *chat);
    QString name() const { return "Contacts management"; };

private:
    QMap<Chat*,ContactsWatcher*> m_watchers;
};

void ContactsPlugin::finalize(Chat *chat)
{
    m_watchers.remove(chat);
}

bool ContactsPlugin::initialize(Chat *chat)
{
    ContactsWatcher *contacts = new ContactsWatcher(chat);
    m_watchers.insert(chat, contacts);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(contacts, ContactsPlugin)

