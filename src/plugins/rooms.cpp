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

#include "QXmppBookmarkManager.h"
#include "QXmppBookmarkSet.h"
#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"
#include "QXmppVCardManager.h"

#include "chat.h"
#include "chat_edit.h"
#include "chat_form.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_utils.h"

#include "rooms.h"

enum MembersColumns {
    JidColumn = 0,
    AffiliationColumn,
};

ChatRoomWatcher::ChatRoomWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    QXmppClient *client = chat->client();

    // add extensions
    bookmarkManager = new QXmppBookmarkManager(client);
    client->addExtension(bookmarkManager);

    mucManager = client->findExtension<QXmppMucManager>();
    if (!mucManager) {
        mucManager = new QXmppMucManager;
        client->addExtension(mucManager);
    }

    bool check;
    check = connect(client, SIGNAL(disconnected()),
                    this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(bookmarkManager, SIGNAL(bookmarksReceived(QXmppBookmarkSet)),
                    this, SLOT(bookmarksReceived()));
    Q_ASSERT(check);

    check = connect(chat->rosterModel(), SIGNAL(ownNameReceived()),
                    this, SLOT(bookmarksReceived()));
    Q_ASSERT(check);

    check = connect(mucManager, SIGNAL(invitationReceived(QString,QString,QString)),
                    this, SLOT(invitationReceived(QString,QString,QString)));
    Q_ASSERT(check);

    check = connect(mucManager, SIGNAL(roomConfigurationReceived(QString,QXmppDataForm)),
                    this, SLOT(roomConfigurationReceived(QString,QXmppDataForm)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(mucServerFound(const QString&)),
                    this, SLOT(mucServerFound(const QString&)));
    Q_ASSERT(check);

    // add roster hooks
    check = connect(chat, SIGNAL(rosterClick(QModelIndex)),
                    this, SLOT(rosterClick(QModelIndex)));
    Q_ASSERT(check);

    check = connect(chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
                    this, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    Q_ASSERT(check);

    check = connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
                    this, SLOT(rosterMenu(QMenu*, QModelIndex)));
    Q_ASSERT(check);

    check = connect(chat, SIGNAL(urlClick(QUrl)),
                    this, SLOT(urlClick(QUrl)));
    Q_ASSERT(check);
 
    // add room button
    roomButton = new QPushButton;
    roomButton->setEnabled(false);
    roomButton->setIcon(QIcon(":/chat.png"));
    roomButton->setText(tr("Rooms"));
    roomButton->setToolTip(tr("Join or create a chat room"));
    connect(roomButton, SIGNAL(clicked()), this, SLOT(roomPrompt()));
    chat->statusBar()->addWidget(roomButton);
}

bool ChatRoomWatcher::bookmarkRoom(const QString &roomJid)
{
    // find bookmark
    QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
    QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
    foreach (const QXmppBookmarkConference &conference, conferences)
    {
        if (conference.jid() == roomJid)
            return true;
    }

    // add bookmark
    QXmppBookmarkConference conference;
    conference.setAutoJoin(true);
    conference.setJid(roomJid);
    conferences << conference;
    bookmarks.setConferences(conferences);
    return bookmarkManager->setBookmarks(bookmarks);
}

bool ChatRoomWatcher::unbookmarkRoom(const QString &roomJid)
{
    // find bookmark
    QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
    QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
    int foundAt = -1;
    for (int i = 0; i < conferences.size(); ++i)
    {
        if (conferences.at(i).jid() == roomJid)
        {
            foundAt = i;
            break;
        }
    }
    if (foundAt < 0)
        return true;

    // remove bookmark
    conferences.removeAt(foundAt);
    bookmarks.setConferences(conferences);
    return bookmarkManager->setBookmarks(bookmarks);
}

void ChatRoomWatcher::bookmarksReceived()
{
    // if we are missing our bookmarks or nickname, don't do anything yet
    if (!chat->rosterModel()->isOwnNameReceived() ||
        !bookmarkManager->areBookmarksReceived())
        return;

    const QXmppBookmarkSet &bookmarks = bookmarkManager->bookmarks();
    foreach (const QXmppBookmarkConference &conference, bookmarks.conferences())
    {
        if (conference.autoJoin())
        {
            // if autojoin is enabled, join room
            joinRoom(conference.jid(), false);
        }
        else
        {
            // otherwise, just add an entry in the roster
            chat->rosterModel()->addItem(ChatRosterItem::Room,
                conference.jid(),
                jidToUser(conference.jid()),
                QIcon(":/room.png"));
        }
    }
}

void ChatRoomWatcher::disconnected()
{
    roomButton->setEnabled(false);
}

ChatRoom *ChatRoomWatcher::joinRoom(const QString &jid, bool focus)
{
    ChatRoom *room = qobject_cast<ChatRoom*>(chat->panel(jid));
    if (!room)
    {
        room = new ChatRoom(chat->client(), chat->rosterModel(), jid);

        // get notified when room is closed
        bool check;
        check = connect(room, SIGNAL(hidePanel()),
                        this, SLOT(roomClose()));
        Q_ASSERT(check);

        // add roster hooks
        check = connect(chat, SIGNAL(rosterClick(QModelIndex)),
                        room, SLOT(rosterClick(QModelIndex)));
        Q_ASSERT(check);

        chat->addPanel(room);
        QMetaObject::invokeMethod(room, "registerPanel");
    }
    room->join();
    if (focus)
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

    // prompt for reason
    const QString roomJid = jidToBareJid(jid);
    const QString roomName = chat->rosterModel()->contactName(roomJid);
    bool ok = false;
    QString reason;
    reason = QInputDialog::getText(chat, tr("Kick user"),
                  tr("Enter the reason for kicking the user from '%1'.").arg(roomName),
                  QLineEdit::Normal, reason, &ok);
    if (!ok)
        return;

    QXmppMucAdminIq::Item item;
    item.setNick(jidToResource(jid));
    item.setRole(QXmppMucAdminIq::Item::NoRole);
    item.setReason(reason);

    QXmppMucAdminIq iq;
    iq.setType(QXmppIq::Set);
    iq.setTo(roomJid);
    iq.setItems(QList<QXmppMucAdminIq::Item>() << item);

    chat->client()->sendPacket(iq);
}

void ChatRoomWatcher::invitationHandled(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    if (box && box->standardButton(button) == QMessageBox::Yes)
    {
        const QString roomJid = box->objectName();
        joinRoom(roomJid, true);
        invitations.removeAll(roomJid);
    }
}

/*** Notify the user when an invitation is received.
 */
void ChatRoomWatcher::invitationReceived(const QString &roomJid, const QString &jid, const QString &reason)
{
    // Skip invitations to rooms which we have already joined or
    // which we have already received
    if (chat->panel(roomJid) || invitations.contains(roomJid))
        return;

    const QString contactName = chat->rosterModel()->contactName(jid);

    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Invitation from %1").arg(jid),
        tr("%1 has asked to add you to join the '%2' chat room.\n\nDo you accept?").arg(contactName, roomJid),
        QMessageBox::Yes | QMessageBox::No,
        chat);
    box->setObjectName(roomJid);
    box->setDefaultButton(QMessageBox::Yes);
    box->setEscapeButton(QMessageBox::No);
    box->open(this, SLOT(invitationHandled(QAbstractButton*)));

    invitations << roomJid;
}

/** When the user closes a chat room, unbookmark it.
 */
void ChatRoomWatcher::roomClose()
{
    ChatRoom *room = qobject_cast<ChatRoom*>(sender());
    if (!room)
        return;
    unbookmarkRoom(room->objectName());
}

/** Prompt the user for a new group chat then join it.
 */
void ChatRoomWatcher::roomPrompt()
{
    ChatRoomPrompt prompt(chat->client(), chatRoomServer, chat);
#ifdef WILINK_EMBEDDED
    prompt.showMaximized();
#endif
    if (!prompt.exec())
        return;
    const QString roomJid = prompt.textValue();
    if (roomJid.isEmpty())
        return;

    // join room and bookmark it
    joinRoom(roomJid, true);
    bookmarkRoom(roomJid);
}

/** Request a chat room's configuration.
 */
void ChatRoomWatcher::roomOptions()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    const QString jid = action->data().toString();
    mucManager->requestRoomConfiguration(jid);
}

/** Request a chat room's members.
 */
void ChatRoomWatcher::roomMembers()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    const QString jid = action->data().toString();

    // manage room members
    ChatRoomMembers dialog(chat->client(), jid, chat);
    dialog.exec();
}

/** Change the room's subject.
 */
void ChatRoomWatcher::roomSubject()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    const QString jid = action->data().toString();

    bool ok;
    QString subject = QInputDialog::getText(chat,
        tr("Change subject"), tr("Subject:"), QLineEdit::Normal,
        QString(), &ok);
    if (ok)
        mucManager->setRoomSubject(jid, subject);
}

/** Display room configuration dialog.
 */
void ChatRoomWatcher::roomConfigurationReceived(const QString &roomJid, const QXmppDataForm &form)
{
    ChatForm dialog(form, chat);
    if (dialog.exec())
        mucManager->setRoomConfiguration(roomJid, dialog.form());
}

/** Once a multi-user chat server is found, enable the "chat rooms" button.
 */
void ChatRoomWatcher::mucServerFound(const QString &mucServer)
{
    chatRoomServer = mucServer;
    roomButton->setEnabled(true);
}

/** Handle a click event on a roster entry.
 */
void ChatRoomWatcher::rosterClick(const QModelIndex &index)
{
    const int type = index.data(ChatRosterModel::TypeRole).toInt();
    if (type != ChatRosterItem::Room)
        return;

    const QString roomJid = index.data(ChatRosterModel::IdRole).toString();
    joinRoom(roomJid, true);
}

/** Handle a drop event on a roster entry.
 */
void ChatRoomWatcher::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

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
            ChatRoom *room = joinRoom(roomJid, true);
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
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Room) {
        int flags = index.data(ChatRosterModel::FlagsRole).toInt();

        if (flags & ChatRosterModel::SubjectFlag)
        {
            QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Change subject"));
            action->setData(jid);
            connect(action, SIGNAL(triggered()), this, SLOT(roomSubject()));
        }

        if (flags & ChatRosterModel::OptionsFlag)
        {
            QAction *action = menu->addAction(QIcon(":/options.png"), tr("Options"));
            action->setData(jid);
            connect(action, SIGNAL(triggered()), this, SLOT(roomOptions()));
        }

        if (flags & ChatRosterModel::MembersFlag)
        {
            QAction *action = menu->addAction(QIcon(":/peer.png"), tr("Permissions"));
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

/** Open a XMPP URI if it refers to a chat room.
 */
void ChatRoomWatcher::urlClick(const QUrl &url)
{
    if (url.scheme() == "xmpp" && url.hasQueryItem("join"))
        joinRoom(url.path(), true);
}

ChatRoom::ChatRoom(ChatClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatConversation(parent),
    chatRemoteJid(jid),
    client(xmppClient),
    joined(false),
    notifyMessages(false),
    rosterModel(chatRosterModel)
{
    nickName = rosterModel->ownName();
    chatLocalJid = jid + "/" + nickName;
    setObjectName(jid);
    setWindowTitle(rosterModel->contactName(jid));
    setWindowIcon(QIcon(":/chat.png"));
    setWindowExtra(jid);
    setWindowHelp(tr("To invite a contact to this chat room, drag and drop it onto the chat room."));

    bool check;
    check = connect(chatInput, SIGNAL(returnPressed()),
                    this, SLOT(returnPressed()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(connected()), this, SLOT(join()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(connected()), this, SIGNAL(registerPanel()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(client->findExtension<QXmppDiscoveryManager>(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    Q_ASSERT(check);

    check = connect(historyWidget(), SIGNAL(messageClicked(ChatMessage)),
                    this, SLOT(messageClicked(ChatMessage)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()), this, SLOT(leave()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(showPanel()), this, SLOT(join()));
    Q_ASSERT(check);

    /* keyboard shortcut */
    connect(chatInput, SIGNAL(tabPressed()), this, SLOT(tabPressed()));
}

/** Handle disconnection from server.
 */
void ChatRoom::disconnected()
{
    joined = false;

    // clear chat room participants
    QModelIndex roomIndex = rosterModel->findItem(chatRemoteJid);
    if (roomIndex.isValid())
        rosterModel->removeRows(0, rosterModel->rowCount(roomIndex), roomIndex);
}

void ChatRoom::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.from() != chatRemoteJid || disco.type() != QXmppIq::Result)
        return;

    // notify user of received messages if the room is not publicly listed
    if (disco.features().contains("muc_hidden"))
        notifyMessages = true;

    // update window title
    setWindowTitle(rosterModel->contactName(chatRemoteJid));
}

/** Invite a user to the chat room.
 */
void ChatRoom::invite(const QString &jid)
{
    if (!client->findExtension<QXmppMucManager>()->sendInvitation(chatRemoteJid, jid, "Let's talk"))
        return;

    // notify user
    queueNotification(tr("%1 has been invited to %2")
        .arg(rosterModel->contactName(jid))
        .arg(rosterModel->contactName(chatRemoteJid)),
        ForceNotification);
}

/** Send a request to join a multi-user chat.
 */
void ChatRoom::join()
{
    if (joined)
        return;

    // clear history
    historyWidget()->clear();

    // send join request
    client->findExtension<QXmppMucManager>()->joinRoom(chatRemoteJid, nickName);

    // request room information
    client->findExtension<QXmppDiscoveryManager>()->requestInfo(chatRemoteJid);

    joined = true;
}

/** Send a request to leave a multi-user chat.
 */
void ChatRoom::leave()
{
    if (joined)
    {
        client->findExtension<QXmppMucManager>()->leaveRoom(chatRemoteJid);
        joined = false;
    }

    /* remove room from roster */
    QModelIndex roomIndex = rosterModel->findItem(chatRemoteJid);
    if (roomIndex.data(ChatRosterModel::PersistentRole).toBool())
        rosterModel->removeRows(0, rosterModel->rowCount(roomIndex), roomIndex);
    else
        rosterModel->removeRow(roomIndex.row(), roomIndex.parent());

    deleteLater();
}

void ChatRoom::messageClicked(const ChatMessage &msg)
{
    QModelIndex roomIndex = rosterModel->findItem(chatRemoteJid);
    for (int i = 0; i < rosterModel->rowCount(roomIndex); i++)
    {
        QModelIndex index = roomIndex.child(i, 0);
        if (index.data(ChatRosterModel::IdRole).toString() == msg.fromJid)
        {
            rosterClick(index);
            break;
        }
    }
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QString from = jidToResource(msg.from());
    if (jidToBareJid(msg.from()) != chatRemoteJid ||
        msg.type() != QXmppMessage::GroupChat)
        return;

    // handle message subject
    if (!msg.subject().isEmpty())
        setWindowStatus(msg.subject());

    // handle message body
    if (!msg.body().isEmpty())
    {
        ChatMessage message;
        message.body = msg.body();
        message.from = from;
        message.fromJid = msg.from();
        message.received = jidToResource(msg.from()) != nickName;
        message.date = msg.stamp();
        if (!message.date.isValid())
            message.date = client->serverTime();
        historyWidget()->addMessage(message);

        // notify user
        if (notifyMessages || message.body.contains("@" + nickName))
            queueNotification(message.body);
    }
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
    if (presence.from() == client->configuration().jid())
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
                    tr("Sorry, but you cannot join chat room '%1'.\n\n%2")
                        .arg(chatRemoteJid)
                        .arg(error.text()));
                break;
            }
        }
        break;
    case QXmppPresence::Unavailable:
        if (jidToResource(presence.from()) != nickName)
            return;

        // leave room
        emit hidePanel();

        foreach (const QXmppElement &extension, presence.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc_user)
            {
                QXmppElement status = extension.firstChildElement("status");
                while (!status.isNull()) {
                    if (status.attribute("code").toInt() == 307) {
                        QXmppElement reason = extension.firstChildElement("item").firstChildElement("reason");
                        QMessageBox::warning(window(),
                            tr("Chat room error"),
                            tr("Sorry, but you were kicked from chat room '%1'.\n\n%2")
                                .arg(chatRemoteJid)
                                .arg(reason.value()));
                        break;
                    }
                    status = status.nextSiblingElement("status");
                }
            }
        }
        break;
    default:
        break;
    }
}

void ChatRoom::rosterClick(const QModelIndex &index)
{
    if (!client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // talk "at" somebody
    if (type == ChatRosterItem::RoomMember && jidToBareJid(jid) == chatRemoteJid && !chatInput->toPlainText().contains("@" + jidToResource(jid) + ": "))
    {
        const QString newAt = "@" + jidToResource(jid);
        QTextCursor cursor = chatInput->textCursor();

        QRegExp rx("((@[^,:]+[,:] )+)");
        QString text = chatInput->document()->toPlainText();
        int oldPos;
        if ((oldPos = rx.indexIn(text)) >= 0)
        {
            QStringList bits = rx.cap(0).split(QRegExp("[,:] "), QString::SkipEmptyParts);
            if (!bits.contains(newAt))
            {
                bits << newAt;
                cursor.setPosition(oldPos);
                cursor.setPosition(oldPos + rx.matchedLength(), QTextCursor::KeepAnchor);
                cursor.insertText(bits.join(", ") + ": ");
            }
        } else {
            cursor.insertText(newAt + ": ");
        }
        emit showPanel();
        chatInput->setFocus();
    }
}

/** Send a message to the chat room.
 */
void ChatRoom::returnPressed()
{
    QString text = chatInput->text();
    if (text.isEmpty())
        return;

    if (client->findExtension<QXmppMucManager>()->sendMessage(chatRemoteJid, text))
        chatInput->clear();
}

void ChatRoom::tabPressed()
{
    QTextCursor cursor = chatInput->textCursor();
    cursor.movePosition(QTextCursor::StartOfWord);
    // FIXME : for some reason on OS X, a leading '@' sign is not considered
    // part of a word
    if (cursor.position() &&
        cursor.document()->characterAt(cursor.position() - 1) == QChar('@')) {
        cursor.setPosition(cursor.position() - 1);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
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
    QLabel *label = new QLabel(tr("Enter the name of the chat room you want to join. If the chat room does not exist yet, it will be created for you."));
    label->setWordWrap(true);
    layout->addWidget(label);
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
    setWindowTitle(tr("Join or create a chat room"));

    // get rooms
    QXmppDiscoveryManager *discoveryManager = client->findExtension<QXmppDiscoveryManager>();
    Q_ASSERT(discoveryManager);

    bool check;
    check = connect(discoveryManager, SIGNAL(itemsReceived(const QXmppDiscoveryIq&)),
                    this, SLOT(discoveryItemsReceived(const QXmppDiscoveryIq&)));
    Q_ASSERT(check);
    discoveryManager->requestItems(chatRoomServer);
}

void ChatRoomPrompt::discoveryItemsReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() == QXmppIq::Result &&
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
    setWindowTitle(tr("Chat room permissions"));

    QXmppMucManager *mucManager = client->findExtension<QXmppMucManager>();
    connect(mucManager, SIGNAL(roomPermissionsReceived(QString, QList<QXmppMucAdminIq::Item>)),
            this, SLOT(roomPermissionsReceived(QString, QList<QXmppMucAdminIq::Item>)));

    affiliations[QXmppMucAdminIq::Item::MemberAffiliation] = tr("member");
    affiliations[QXmppMucAdminIq::Item::AdminAffiliation] = tr("administrator");
    affiliations[QXmppMucAdminIq::Item::OwnerAffiliation] = tr("owner");
    affiliations[QXmppMucAdminIq::Item::OutcastAffiliation] = tr("banned");
    mucManager->requestRoomPermissions(chatRoomJid);
}

void ChatRoomMembers::roomPermissionsReceived(const QString &roomJid, const QList<QXmppMucAdminIq::Item> &permissions)
{
    if (roomJid != chatRoomJid)
        return;
    foreach (const QXmppMucAdminIq::Item &item, permissions)
    {
        const QString jid = item.jid();
        QXmppMucAdminIq::Item::Affiliation affiliation = item.affiliation();
        if (!initialMembers.contains(jid))
        {
            addEntry(jid, affiliation);
            initialMembers[jid] = affiliation;
        }
    }
    tableWidget->sortItems(JidColumn, Qt::AscendingOrder);;
}

void ChatRoomMembers::submit()
{
    QList<QXmppMucAdminIq::Item> items;

    // Process changed members
    for (int i = 0; i < tableWidget->rowCount(); i++)
    {
        const QComboBox *combo = qobject_cast<QComboBox *>(tableWidget->cellWidget(i, AffiliationColumn));
        Q_ASSERT(tableWidget->item(i, JidColumn) && combo);

        const QString currentJid = tableWidget->item(i, JidColumn)->text();
        QXmppMucAdminIq::Item::Affiliation currentAffiliation = static_cast<QXmppMucAdminIq::Item::Affiliation>(combo->itemData(combo->currentIndex()).toInt());
        if (initialMembers.value(currentJid) != currentAffiliation)
        {
            QXmppMucAdminIq::Item item;
            item.setAffiliation(currentAffiliation);
            item.setJid(currentJid);
            items.append(item);
        }
        initialMembers.remove(currentJid);
    }

    // Process deleted members (i.e. remaining entries in initialMember)
    foreach(const QString &entry, initialMembers.keys())
    {
        QXmppMucAdminIq::Item item;
        item.setAffiliation(QXmppMucAdminIq::Item::NoAffiliation);
        item.setJid(entry);
        items.append(item);
    }

    if (!items.isEmpty())
    {
        QXmppMucAdminIq iq;
        iq.setTo(chatRoomJid);
        iq.setType(QXmppIq::Set);
        iq.setItems(items);
        client->sendPacket(iq);
    }
    accept();
}

void ChatRoomMembers::addMember()
{
    bool ok = false;
    QString jid = "@" + client->configuration().domain();
    jid = QInputDialog::getText(this, tr("Add a user"),
                  tr("Enter the address of the user you want to add."),
                  QLineEdit::Normal, jid, &ok).toLower();
    if (ok)
        addEntry(jid, QXmppMucAdminIq::Item::MemberAffiliation);
}

void ChatRoomMembers::addEntry(const QString &jid, QXmppMucAdminIq::Item::Affiliation affiliation)
{
    QComboBox *combo = new QComboBox;
    foreach (QXmppMucAdminIq::Item::Affiliation key, affiliations.keys())
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
    new ChatRoomWatcher(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(rooms, RoomsPlugin)

