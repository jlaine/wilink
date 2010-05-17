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

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextBlock>
#include <QTimer>
#include <QUrl>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppMucIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_edit.h"
#include "chat_form.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "utils.h"

#include "rooms.h"

enum MembersColumns {
    JidColumn = 0,
    AffiliationColumn,
};

ChatRoomWatcher::ChatRoomWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    ChatClient *client = chat->client();
    connect(client, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(client, SIGNAL(messageReceived(QXmppMessage)),
            this, SLOT(messageReceived(QXmppMessage)));
    connect(client, SIGNAL(mucOwnerIqReceived(const QXmppMucOwnerIq&)),
            this, SLOT(mucOwnerIqReceived(const QXmppMucOwnerIq&)));
    connect(client, SIGNAL(mucServerFound(const QString&)),
            this, SLOT(mucServerFound(const QString&)));

    // add roster hooks
    connect(chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
            this, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            this, SLOT(rosterMenu(QMenu*, QModelIndex)));
 
    // add room button
    roomButton = new QPushButton;
    roomButton->setEnabled(false);
    roomButton->setIcon(QIcon(":/chat.png"));
    roomButton->setToolTip(tr("Join a chat room"));
    connect(roomButton, SIGNAL(clicked()), this, SLOT(roomJoin()));
    chat->statusBar()->addWidget(roomButton);
}

void ChatRoomWatcher::disconnected()
{
    roomButton->setEnabled(false);
}

/** Invite a contact to join a chat room.
 */
void ChatRoomWatcher::inviteContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    // prompt the user for chat room
    ChatRoomPrompt prompt(chat->client(), chatRoomServer, chat);
    if (!prompt.exec())
        return;

    // join chat room and invite contact
    const QString roomJid = prompt.textValue();
    ChatRoom *room = joinRoom(roomJid);
    room->invite(jid);
}

ChatRoom *ChatRoomWatcher::joinRoom(const QString &jid)
{
    ChatRoom *room = qobject_cast<ChatRoom*>(chat->panel(jid));
    if (!room)
    {
        room = new ChatRoom(chat->client(), chat->rosterModel(), jid);
        // add roster hooks
        connect(chat, SIGNAL(rosterClick(QModelIndex)),
            room, SLOT(rosterClick(QModelIndex)));
        chat->addPanel(room);
    }
    QTimer::singleShot(0, room, SIGNAL(showPanel()));
    return room;
}

/** Kick a user from a chat room.
 */
void ChatRoomWatcher::kickUser()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();


    QXmppElement item;
    item.setTagName("item");
    item.setAttribute("nick", jidToResource(jid));
    item.setAttribute("role", "none");

    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_admin);
    query.appendChild(item);

    QXmppIq iq(QXmppIq::Set);
    iq.setTo(jidToBareJid(jid));
    iq.setExtensions(query);

    chat->client()->sendPacket(iq);
}

void ChatRoomWatcher::messageHandled(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    if (box && box->standardButton(button) == QMessageBox::Yes)
    {
        const QString roomJid = box->objectName();
        joinRoom(roomJid);
        invitations.removeAll(roomJid);
    }
}

void ChatRoomWatcher::messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Normal)
        return;

    // process room invitations
    const QString bareJid = jidToBareJid(msg.from());
    foreach (const QXmppElement &extension, msg.extensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
        {
            const QString contactName = chat->rosterModel()->contactName(bareJid);
            const QString roomJid = extension.attribute("jid");
            if (!roomJid.isEmpty() && !chat->panel(roomJid) && !invitations.contains(roomJid))
            {
                QMessageBox *box = new QMessageBox(QMessageBox::Question,
                    tr("Invitation from %1").arg(bareJid),
                    tr("%1 has asked to add you to join the '%2' chat room.\n\nDo you accept?").arg(contactName, roomJid),
                    QMessageBox::Yes | QMessageBox::No,
                    chat);
                box->setObjectName(roomJid);
                box->setDefaultButton(QMessageBox::Yes);
                box->setEscapeButton(QMessageBox::No);
                box->open(this, SLOT(messageHandled(QAbstractButton*)));

                invitations << roomJid;
            }
            break;
        }
    }
}

/** Prompt the user for a new group chat then join it.
 */
void ChatRoomWatcher::roomJoin()
{
    ChatRoomPrompt prompt(chat->client(), chatRoomServer, chat);
    if (!prompt.exec())
        return;
    joinRoom(prompt.textValue());
}

/** Display a chat room's options.
 */
void ChatRoomWatcher::roomOptions()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    // get room information
    QXmppIq iq;
    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_owner);
    iq.setExtensions(query);
    iq.setTo(jid);
    chat->client()->sendPacket(iq);
}

void ChatRoomWatcher::roomMembers()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString jid = action->data().toString();

    // manage room members
    ChatRoomMembers dialog(chat->client(), jid, chat);
    dialog.exec();
}

void ChatRoomWatcher::mucOwnerIqReceived(const QXmppMucOwnerIq &iq)
{
    if (iq.type() != QXmppIq::Result || iq.form().isNull())
        return;

    ChatForm dialog(iq.form(), chat);
    if (dialog.exec())
    {
        QXmppMucOwnerIq iqPacket;
        iqPacket.setType(QXmppIq::Set);
        iqPacket.setTo(iq.from());
        iqPacket.setForm(dialog.form());
        chat->client()->sendPacket(iqPacket);
    }
}

/** Once a multi-user chat server is found, enable the "chat rooms" button.
 */
void ChatRoomWatcher::mucServerFound(const QString &mucServer)
{
    chatRoomServer = mucServer;
    roomButton->setEnabled(true);
}

/** Handle a drop event on a roster entry.
 */
void ChatRoomWatcher::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    int type = index.data(ChatRosterModel::TypeRole).toInt();
    if (type != ChatRosterItem::Room || !event->mimeData()->hasUrls())
        return;

    const QString roomJid = index.data(ChatRosterModel::IdRole).toString();
    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        const QString jid = url.toString();
        if (!isBareJid(jid))
            continue;
        if (event->type() == QEvent::Drop)
        {
            ChatRoom *room = joinRoom(roomJid);
            foreach (const QUrl &url, event->mimeData()->urls())
                room->invite(jid);
        }
        found++;
    }
    if (found)
        event->acceptProposedAction();
}

/** Add entries to a roster entry's context menu.
 */
void ChatRoomWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Contact)
    {
        QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Invite to a chat room"));
        action->setData(jid);
        connect(action, SIGNAL(triggered()), this, SLOT(inviteContact()));
    } else if (type == ChatRosterItem::Room) {
        int flags = index.data(ChatRosterModel::FlagsRole).toInt();
        if (flags & ChatRosterModel::OptionsFlag)
        {
            QAction *action = menu->addAction(QIcon(":/options.png"), tr("Options"));
            action->setData(jid);
            connect(action, SIGNAL(triggered()), this, SLOT(roomOptions()));
        }

        if (flags & ChatRosterModel::MembersFlag)
        {
            QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Members"));
            action->setData(jid);
            connect(action, SIGNAL(triggered()), this, SLOT(roomMembers()));
        }
    } else if (type == ChatRosterItem::RoomMember) {
        QModelIndex room = index.parent();
        if (room.data(ChatRosterModel::FlagsRole).toInt() & ChatRosterModel::KickFlag)
        {
            QAction *action = menu->addAction(QIcon(":/remove.png"), tr("Kick user"));
            action->setData(jid);
            connect(action, SIGNAL(triggered()), this, SLOT(kickUser()));
        }
    }
}

ChatRoom::ChatRoom(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatConversation(jid, parent),
    client(xmppClient),
    joined(false),
    notifyMessages(false),
    rosterModel(chatRosterModel)
{
    chatLocalJid = jid + "/" + rosterModel->ownName();
    setWindowTitle(rosterModel->contactName(jid));
    setWindowIcon(QIcon(":/chat.png"));

    /* help label */
    QVBoxLayout *vbox = qobject_cast<QVBoxLayout*>(layout());
    int index = vbox->indexOf(chatHistory);
    vbox->insertSpacing(index++, 10);
    QLabel *helpLabel = new QLabel(tr("To invite a contact to this chat room, drag and drop it onto the chat room."));
    helpLabel->setWordWrap(true);
    vbox->insertWidget(index++, helpLabel);
    vbox->insertSpacing(index++, 10);

    connect(client, SIGNAL(connected()), this, SLOT(join()));
    connect(client, SIGNAL(connected()), this, SIGNAL(registerPanel()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(client, SIGNAL(discoveryIqReceived(QXmppDiscoveryIq)), this, SLOT(discoveryIqReceived(QXmppDiscoveryIq)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(this, SIGNAL(hidePanel()), this, SLOT(leave()));
    connect(this, SIGNAL(hidePanel()), this, SIGNAL(unregisterPanel()));
    connect(this, SIGNAL(showPanel()), this, SLOT(join()));

    /* keyboard shortcut */
    connect(chatInput, SIGNAL(tabPressed()), this, SLOT(tabPressed()));
}

void ChatRoom::disconnected()
{
    joined = false;
}

void ChatRoom::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.from() != chatRemoteJid ||
        disco.type() != QXmppIq::Result ||
        disco.queryType() != QXmppDiscoveryIq::InfoQuery)
        return;

    // notify user of received messages if the room is not publicly listed
    if (disco.features().contains("muc_hidden"))
        notifyMessages = true;
}

/** Invite a user to the chat room.
 */
void ChatRoom::invite(const QString &jid)
{
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_conference);
    x.setAttribute("jid", chatRemoteJid);
    x.setAttribute("reason", "Let's talk");

    QXmppMessage message;
    message.setTo(jid);
    message.setType(QXmppMessage::Normal);
    message.setExtensions(x);
    client->sendPacket(message);

    // let user know about the invitation
    emit notifyPanel(tr("%1 has been invited to %2")
        .arg(rosterModel->contactName(jid))
        .arg(rosterModel->contactName(chatRemoteJid)));
}

/** Send a request to join a multi-user chat.
 */
void ChatRoom::join()
{
    if (joined)
        return;

    // clear history
    chatHistory->clear();

    // request room information
    QXmppDiscoveryIq disco;
    disco.setTo(chatRemoteJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    client->sendPacket(disco);
    
    // send join request
    QXmppPresence packet;
    packet.setTo(chatLocalJid);
    packet.setType(QXmppPresence::Available);
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_muc);
    packet.setExtensions(x);
    client->sendPacket(packet);
    
    joined = true;
}

/** Send a request to leave a multi-user chat.
 */
void ChatRoom::leave()
{
    if (joined)
    {
        QXmppPresence packet;
        packet.setTo(chatLocalJid);
        packet.setType(QXmppPresence::Unavailable);
        client->sendPacket(packet);
        joined = false;
    }
    deleteLater();
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QString from = jidToResource(msg.from());
    if (jidToBareJid(msg.from()) != chatRemoteJid || from.isEmpty())
        return;

    ChatHistoryMessage message;
    message.body = msg.body();
    message.from = from;
    message.fromJid = msg.from();
    message.received = (msg.from() != chatLocalJid);
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
    chatHistory->addMessage(message);

    // notify
    if (notifyMessages)
        emit notifyPanel(message.body);
}

/** Return the type of entry to add to the roster.
 */
ChatRosterItem::Type ChatRoom::objectType() const
{
    return ChatRosterItem::Room;
}

void ChatRoom::presenceReceived(const QXmppPresence &presence)
{
    // if our own presence changes, reflect it in the chat room
    if (presence.from() == client->getConfiguration().jid())
    {
        QXmppPresence packet;
        packet.setTo(chatLocalJid);
        packet.setType(presence.type());
        packet.setStatus(presence.status());
        client->sendPacket(packet);
    }

    if (jidToBareJid(presence.from()) != chatRemoteJid)
        return;

    switch (presence.type())
    {
    case QXmppPresence::Error:
        foreach (const QXmppElement &extension, presence.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc)
            {
                // leave room
                joined = false;
                emit hidePanel();

                QXmppStanza::Error error = presence.error();
                QMessageBox::warning(window(),
                    tr("Chat room error"),
                    tr("Sorry, but you cannot join chat room %1.\n\n%2")
                        .arg(chatRemoteJid)
                        .arg(error.text()));
                break;
            }
        }
        break;
    case QXmppPresence::Unavailable:
        if (presence.from() != chatLocalJid)
            return;

        // leave room
        emit hidePanel();

        foreach (const QXmppElement &extension, presence.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc_user)
            {
                int statusCode = extension.firstChildElement("status").attribute("code").toInt();
                if (statusCode == 307)
                {
                    QXmppElement reason = extension.firstChildElement("item").firstChildElement("reason");
                    QMessageBox::warning(window(),
                        tr("Chat room error"),
                        tr("Sorry, but you were kicked from chat room %1.\n\n%2")
                            .arg(chatRemoteJid)
                            .arg(reason.value()));
                }
                break;
            }
        }
        break;
    }
}

void ChatRoom::rosterClick(const QModelIndex &index)
{
    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // talk "at" somebody
    if (type == ChatRosterItem::RoomMember && jidToBareJid(jid) == chatRemoteJid)
    {
        chatInput->textCursor().insertText("@" + jidToResource(jid) + ": ");
        chatInput->setFocus();
    }
}

void ChatRoom::sendMessage(const QString &text)
{
    QXmppMessage msg;
    msg.setBody(text);
    msg.setFrom(chatLocalJid);
    msg.setTo(chatRemoteJid);
    msg.setType(QXmppMessage::GroupChat);
    client->sendPacket(msg);
}

void ChatRoom::tabPressed()
{
    QTextCursor cursor = chatInput->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString prefix = cursor.selectedText().toLower();
    if (prefix.startsWith("@"))
        prefix = prefix.mid(1);

    /* find matching room members */
    QStringList matches;
    QModelIndex roomIndex = rosterModel->findItem(chatRemoteJid);
    for (int i = 0; i < rosterModel->rowCount(roomIndex); i++)
    {
        QString member = roomIndex.child(i, 0).data(Qt::DisplayRole).toString();
        if (member.toLower().startsWith(prefix))
            matches << member;
    }
    if (matches.size() == 1)
        cursor.insertText("@" + matches[0] + ": ");
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
    if (disco.type() == QXmppIq::Result &&
        disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == chatRoomServer)
    {
        // chat rooms list
        listWidget->clear();
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), item.name());
            wdgItem->setData(Qt::UserRole, item.jid());
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

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderItem(JidColumn, new QTableWidgetItem(tr("User")));
    tableWidget->setHorizontalHeaderItem(AffiliationColumn, new QTableWidgetItem(tr("Role")));
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setResizeMode(JidColumn, QHeaderView::Stretch);

    layout->addWidget(tableWidget);

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

    setWindowTitle(tr("Chat room members"));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));

    affiliations["member"] = tr("member");
    affiliations["admin"] = tr("administrator");
    affiliations["owner"] = tr("owner");
    affiliations["outcast"] = tr("banned");
    foreach (const QString &affiliation, affiliations.keys())
    {
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("affiliation", affiliation);

        QXmppElement query;
        query.setTagName("query");
        query.setAttribute("xmlns", ns_muc_admin);
        query.appendChild(item);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setExtensions(query);
        client->sendPacket(iq);
    }
}

void ChatRoomMembers::iqReceived(const QXmppIq &iq)
{
    if (iq.type() != QXmppIq::Result ||
        iq.from() != chatRoomJid)
        return;
    const QXmppElement query = iq.extensions().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_admin)
        return;

    QXmppElement element = query.firstChildElement();
    while (!element.isNull())
    {
        const QString jid = element.attribute("jid");
        const QString affiliation = element.attribute("affiliation");
        if (!initialMembers.contains(jid))
        {
            addEntry(jid, affiliation);
            initialMembers[jid] = affiliation;
        }
        element = element.nextSiblingElement();
    }
    tableWidget->sortItems(JidColumn, Qt::AscendingOrder);;
}

void ChatRoomMembers::submit()
{
    QXmppElementList elements;
    for (int i=0; i < tableWidget->rowCount(); i++)
    {
        const QComboBox *combo = qobject_cast<QComboBox *>(tableWidget->cellWidget(i, AffiliationColumn));
        Q_ASSERT(tableWidget->item(i, JidColumn) && combo);

        const QString currentJid = tableWidget->item(i, JidColumn)->text();
        const QString currentAffiliation = combo->itemData(combo->currentIndex()).toString();
        if (initialMembers.value(currentJid) != currentAffiliation) {
            QXmppElement item;
            item.setTagName("item");
            item.setAttribute("affiliation", currentAffiliation);
            item.setAttribute("jid", currentJid);
            elements.append(item);
        }
        initialMembers.remove(currentJid);
    }

    // Process deleted members (i.e. remaining entries in initialMember)
    foreach(const QString &entry, initialMembers.keys())
    {
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("affiliation", "none");
        item.setAttribute("jid", entry);
        elements.append(item);
    }

    if (! elements.isEmpty()) {
        QXmppElement query;
        query.setTagName("query");
        query.setAttribute("xmlns", ns_muc_admin);
        foreach (const QXmppElement &child, elements)
            query.appendChild(child);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setType(QXmppIq::Set);
        iq.setExtensions(query);
        client->sendPacket(iq);
    }
    accept();
}

void ChatRoomMembers::addMember()
{
    bool ok = false;
    QString jid = "@" + client->getConfiguration().domain();
    jid = QInputDialog::getText(this, tr("Add a user"),
                  tr("Enter the address of the user you want to add."),
                  QLineEdit::Normal, jid, &ok).toLower();
    if (ok)
        addEntry(jid, "member");
}

void ChatRoomMembers::addEntry(const QString &jid, const QString &affiliation)
{
    QComboBox *combo = new QComboBox;
    foreach (const QString &key, affiliations.keys())
        combo->addItem(affiliations[key], key);
    combo->setEditable(false);
    combo->setCurrentIndex(combo->findData(affiliation));
    QTableWidgetItem *jidItem = new QTableWidgetItem(jid);
    jidItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    tableWidget->insertRow(0);
    tableWidget->setCellWidget(0, AffiliationColumn, combo);
    tableWidget->setItem(0, JidColumn, jidItem);
}

void ChatRoomMembers::removeMember()
{
    tableWidget->removeRow(tableWidget->currentRow());
}

// PLUGIN

class RoomsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool RoomsPlugin::initialize(Chat *chat)
{
    ChatRoomWatcher *rooms = new ChatRoomWatcher(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(rooms, RoomsPlugin)

