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

#include "QSoundPlayer.h"

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

#include "application.h"
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
    QXmppBookmarkManager *bookmarkManager = client->findExtension<QXmppBookmarkManager>();
    if (!bookmarkManager) {
        bookmarkManager = new QXmppBookmarkManager(client);
        client->addExtension(bookmarkManager);
    }

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

    check = connect(mucManager, SIGNAL(invitationReceived(QString,QString,QString)),
                    this, SLOT(invitationReceived(QString,QString,QString)));
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

void ChatRoomWatcher::bookmarksReceived()
{
    QXmppBookmarkManager *bookmarkManager = chat->client()->findExtension<QXmppBookmarkManager>();
    Q_ASSERT(bookmarkManager);

    // join rooms marked as "autojoin"
    const QXmppBookmarkSet &bookmarks = bookmarkManager->bookmarks();
    foreach (const QXmppBookmarkConference &conference, bookmarks.conferences()) {
        if (conference.autoJoin())
            joinRoom(conference.jid(), false);
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
        room = new ChatRoom(chat, chat->rosterModel(), jid);

        // add roster hooks
        bool check;
        check = connect(chat, SIGNAL(rosterClick(QModelIndex)),
                        room, SLOT(rosterClick(QModelIndex)));
        Q_ASSERT(check);
        Q_UNUSED(check);

        // add panel
        chat->addPanel(room);
        chat->rosterModel()->addItem(room->objectType(), room->objectName(),
                                     room->windowTitle(), QPixmap(":/chat.png"));
    }
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

    QXmppMucRoom *mucRoom = mucManager->addRoom(roomJid);
    if (mucRoom)
        mucRoom->kick(jid, reason);
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
    Q_UNUSED(reason);

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
    ChatRoom *room = joinRoom(roomJid, true);
    room->bookmark();
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
    if (type != ChatRosterModel::Room)
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
    if (type != ChatRosterModel::Room || !event->mimeData()->hasUrls())
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
    if (type == ChatRosterModel::RoomMember) {
        const QString jid = index.data(ChatRosterModel::IdRole).toString();
        const QString roomJid = index.parent().data(ChatRosterModel::IdRole).toString();

        QXmppMucRoom *mucRoom = mucManager->addRoom(roomJid);
        if (mucRoom && (mucRoom->allowedActions() & QXmppMucRoom::KickAction)) {
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

ChatRoom::ChatRoom(Chat *chatWindow, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatConversation(parent),
    chat(chatWindow),
    notifyMessages(false),
    rosterModel(chatRosterModel)
{
    bool check;
    QXmppClient *client = chat->client();

    setObjectName(jid);
    setRosterModel(rosterModel);
    setWindowTitle(rosterModel->contactName(jid));
    setWindowIcon(QIcon(":/chat.png"));
    setWindowExtra(jid);
    setWindowHelp(tr("To invite a contact to this chat room, drag and drop it onto the chat room."));

    mucRoom = client->findExtension<QXmppMucManager>()->addRoom(jid);

    // add actions
    subjectAction = addAction(QIcon(":/chat.png"), tr("Subject"));
    check = connect(subjectAction, SIGNAL(triggered()),
                    this, SLOT(changeSubject()));
    Q_ASSERT(check);
    subjectAction->setVisible(false);

    optionsAction = addAction(QIcon(":/options.png"), tr("Options"));
    check = connect(optionsAction, SIGNAL(triggered()),
                    mucRoom, SLOT(requestConfiguration()));
    Q_ASSERT(check);
    optionsAction->setVisible(false);

    permissionsAction = addAction(QIcon(":/peer.png"), tr("Permissions"));
    check = connect(permissionsAction, SIGNAL(triggered()),
                    this, SLOT(changePermissions()));
    Q_ASSERT(check);
    permissionsAction->setVisible(false);

    check = connect(chatRoomList, SIGNAL(clicked(QModelIndex)),
                    this, SLOT(participantClicked(QModelIndex)));
    Q_ASSERT(check);

    // connect signals
    check = connect(chatInput, SIGNAL(returnPressed()),
                    this, SLOT(returnPressed()));
    Q_ASSERT(check);

    // get notified when room is closed
    check = connect(this, SIGNAL(hidePanel()),
                    this, SLOT(unbookmark()));
    Q_ASSERT(check);

    check = connect(rosterModel, SIGNAL(ownNameReceived()), this, SLOT(join()));
    Q_ASSERT(check);

    check = connect(client->findExtension<QXmppDiscoveryManager>(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(configurationReceived(QXmppDataForm)),
                    this, SLOT(configurationReceived(QXmppDataForm)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(error(QXmppStanza::Error)),
                    this, SLOT(error(QXmppStanza::Error)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(kicked(QString,QString)),
                    this, SLOT(kicked(QString,QString)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(joined()), 
                    this, SLOT(joined()));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(left()),
                    this, SLOT(left()));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(participantAdded(QString)),
                    this, SLOT(participantAdded(QString)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(participantChanged(QString)),
                    this, SLOT(participantChanged(QString)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(participantRemoved(QString)),
                    this, SLOT(participantRemoved(QString)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(subjectChanged(QString)),
                    this, SLOT(subjectChanged(QString)));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(messageReceived(QXmppMessage)),
                    this, SLOT(messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(messageClicked(QModelIndex)),
                    this, SLOT(onMessageClicked(QModelIndex)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
                    mucRoom, SLOT(leave()));
    Q_ASSERT(check);

    /* keyboard shortcut */
    connect(chatInput, SIGNAL(tabPressed()), this, SLOT(tabPressed()));

    /* if nickname is received, join now */
    if (rosterModel->isOwnNameReceived())
        join();
}

/** Bookmarks the room.
 */
void ChatRoom::bookmark()
{
    QXmppBookmarkManager *bookmarkManager = chat->client()->findExtension<QXmppBookmarkManager>();
    Q_ASSERT(bookmarkManager);

    // find bookmark
    QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
    QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
    foreach (const QXmppBookmarkConference &conference, conferences) {
        if (conference.jid() == mucRoom->jid())
            return;
    }

    // add bookmark
    QXmppBookmarkConference conference;
    conference.setAutoJoin(true);
    conference.setJid(mucRoom->jid());
    conferences << conference;
    bookmarks.setConferences(conferences);
    bookmarkManager->setBookmarks(bookmarks);
}

/** Manage the room's members.
 */
void ChatRoom::changePermissions()
{
    ChatRoomMembers dialog(mucRoom, "@" + chat->client()->configuration().domain(), chat);
    dialog.exec();
}

/** Change the room's subject.
 */
void ChatRoom::changeSubject()
{
    bool ok;
    QString subject = QInputDialog::getText(chat,
        tr("Change subject"), tr("Subject:"), QLineEdit::Normal,
        QString(), &ok);
    if (ok)
        mucRoom->setSubject(subject);
}

/** Display room configuration dialog.
 */
void ChatRoom::configurationReceived(const QXmppDataForm &form)
{
    ChatForm dialog(form, chat);
    if (dialog.exec())
        mucRoom->setConfiguration(dialog.form());
}

void ChatRoom::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.from() != mucRoom->jid() || disco.type() != QXmppIq::Result)
        return;

    // notify user of received messages if the room is not publicly listed
    if (disco.features().contains("muc_hidden"))
        notifyMessages = true;

    // update window title
    setWindowTitle(rosterModel->contactName(mucRoom->jid()));
}

/** Handle an error.
 */
void ChatRoom::error(const QXmppStanza::Error &error)
{
    QMessageBox::warning(window(),
        tr("Chat room error"),
        tr("Sorry, but you cannot join chat room '%1'.\n\n%2")
            .arg(mucRoom->jid())
            .arg(error.text()));
}

/** Invite a user to the chat room.
 */
void ChatRoom::invite(const QString &jid)
{
    if (!mucRoom->sendInvitation(jid, "Let's talk"))
        return;

    // notify user
    queueNotification(tr("%1 has been invited to %2")
        .arg(rosterModel->contactName(jid))
        .arg(rosterModel->contactName(mucRoom->jid())),
        ForceNotification);
}

/** Send a request to join a multi-user chat.
 */
void ChatRoom::join()
{
    if (mucRoom->isJoined())
        return;

    QXmppClient *client = chat->client();

    // clear history
    historyModel()->clear();

    // send join request
    mucRoom->setNickName(rosterModel->ownName());
    mucRoom->join();

    // request room information
    client->findExtension<QXmppDiscoveryManager>()->requestInfo(mucRoom->jid());
}

void ChatRoom::joined()
{
    // set action visibility
    QXmppMucRoom::Actions actions = mucRoom->allowedActions();
    subjectAction->setVisible(actions & QXmppMucRoom::SubjectAction);
    optionsAction->setVisible(actions & QXmppMucRoom::ConfigurationAction);
    permissionsAction->setVisible(actions & QXmppMucRoom::PermissionsAction);

    // display participants
    QModelIndex roomIndex = rosterModel->findItem(mucRoom->jid());
    chatRoomList->setRootIndex(roomIndex);
    chatRoomList->show();
}

void ChatRoom::kicked(const QString &jid, const QString &reason)
{
    Q_UNUSED(jid);
    QMessageBox::warning(window(),
        tr("Chat room error"),
        tr("Sorry, but you were kicked from chat room '%1'.\n\n%2")
            .arg(mucRoom->jid())
            .arg(reason));
}

/** Handle leaving the room.
 */
void ChatRoom::left()
{
    // remove room from roster unless it's persistent
    QModelIndex roomIndex = rosterModel->findItem(mucRoom->jid());
    if (!roomIndex.data(ChatRosterModel::PersistentRole).toBool())
        rosterModel->removeRow(roomIndex.row(), roomIndex.parent());

    // destroy window
    deleteLater();
}

void ChatRoom::onMessageClicked(const QModelIndex &messageIndex)
{
    const QString fromJid = messageIndex.data(ChatHistoryModel::JidRole).toString();
    QModelIndex roomIndex = rosterModel->findItem(mucRoom->jid());
    for (int i = 0; i < rosterModel->rowCount(roomIndex); i++) {
        QModelIndex index = roomIndex.child(i, 0);
        if (index.data(ChatRosterModel::IdRole).toString() == fromJid) {
            rosterClick(index);
            break;
        }
    }
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    if (msg.body().isEmpty())
        return;

    // handle message body
    ChatMessage message;
    message.body = msg.body();
    message.date = msg.stamp();
    if (!message.date.isValid())
        message.date = chat->client()->serverTime();
    message.jid = msg.from();
    message.received = jidToResource(msg.from()) != mucRoom->nickName();
    historyModel()->addMessage(message);

    // notify user
    if (notifyMessages || message.body.contains("@" + mucRoom->nickName()))
        queueNotification(message.body);

    // play sound, unless we sent the message
    if (message.received)
        wApp->soundPlayer()->play(wApp->incomingMessageSound());
}

void ChatRoom::participantAdded(const QString &jid)
{
    //qDebug("participant added %s", qPrintable(jid));
    QModelIndex roomIndex = rosterModel->findItem(mucRoom->jid());
    QModelIndex index = rosterModel->addItem(ChatRosterModel::RoomMember, jid, jidToResource(jid), QPixmap(":/peer.png"), roomIndex);
    if (index.isValid())
        rosterModel->setData(index, mucRoom->participantPresence(jid).status().type(), ChatRosterModel::StatusRole);

    // FIXME : update number of users
}

void ChatRoom::participantChanged(const QString &jid)
{
    //qDebug("participant changed %s", qPrintable(jid));
    QModelIndex index = rosterModel->findItem(jid);
    if (index.isValid())
        rosterModel->setData(index, mucRoom->participantPresence(jid).status().type(), ChatRosterModel::StatusRole);
}

void ChatRoom::participantClicked(const QModelIndex index)
{
    if(!index.isValid())
        return;

    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    //qDebug("participant clicked %s", qPrintable(jid));

    // notify plugins
    emit rosterClick(index);
}

void ChatRoom::participantRemoved(const QString &jid)
{
    //qDebug("participant removed %s", qPrintable(jid));
    QModelIndex index = rosterModel->findItem(jid);
    if (index.isValid())
        rosterModel->removeRow(index.row(), index.parent());

    // FIXME : update number of users
}

/** Return the type of entry to add to the roster.
 */
ChatRosterModel::Type ChatRoom::objectType() const
{
    return ChatRosterModel::Room;
}

void ChatRoom::rosterClick(const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    const QString roomJid = mucRoom->jid();

    // talk "at" somebody
    if (type == ChatRosterModel::RoomMember && jidToBareJid(jid) == roomJid && !chatInput->toPlainText().contains("@" + jidToResource(jid) + ": "))
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

void ChatRoom::subjectChanged(const QString &subject)
{
    setWindowStatus(subject);
}

/** Send a message to the chat room.
 */
void ChatRoom::returnPressed()
{
    QString text = chatInput->text();
    if (text.isEmpty())
        return;

    // try to send message
    if (!mucRoom->sendMessage(text))
        return;

    // clear input
    chatInput->clear();

    // play sound
    wApp->soundPlayer()->play(wApp->outgoingMessageSound());
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
    QModelIndex roomIndex = rosterModel->findItem(mucRoom->jid());
    for (int i = 0; i < rosterModel->rowCount(roomIndex); i++) {
        QString member = roomIndex.child(i, 0).data(Qt::DisplayRole).toString();
        if (member.toLower().startsWith(prefix))
            matches << member;
    }
    if (matches.size() == 1)
        cursor.insertText("@" + matches[0] + ": ");
}

/** Unbookmarks the room.
 */
void ChatRoom::unbookmark()
{
    QXmppBookmarkManager *bookmarkManager = chat->client()->findExtension<QXmppBookmarkManager>();
    Q_ASSERT(bookmarkManager);

    // find bookmark
    QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
    QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
    for (int i = 0; i < conferences.size(); ++i) {
        if (conferences.at(i).jid() == mucRoom->jid()) {
            // remove bookmark
            conferences.removeAt(i);
            bookmarks.setConferences(conferences);
            bookmarkManager->setBookmarks(bookmarks);
            return;
        }
    }
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

ChatRoomMembers::ChatRoomMembers(QXmppMucRoom *mucRoom, const QString &defaultJid, QWidget *parent)
    : QDialog(parent),
    m_defaultJid(defaultJid),
    m_room(mucRoom)
{
    QVBoxLayout *layout = new QVBoxLayout;

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderItem(JidColumn, new QTableWidgetItem(tr("User")));
    m_tableWidget->setHorizontalHeaderItem(AffiliationColumn, new QTableWidgetItem(tr("Role")));
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->horizontalHeader()->setResizeMode(JidColumn, QHeaderView::Stretch);

    layout->addWidget(m_tableWidget);

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

    // request current permissions
    connect(m_room, SIGNAL(permissionsReceived(QList<QXmppMucAdminIq::Item>)),
            this, SLOT(permissionsReceived(QList<QXmppMucAdminIq::Item>)));
    m_room->requestPermissions();
}

void ChatRoomMembers::permissionsReceived(const QList<QXmppMucAdminIq::Item> &permissions)
{
    foreach (const QXmppMucAdminIq::Item &item, permissions)
        addEntry(item.jid(), item.affiliation());
    m_tableWidget->sortItems(JidColumn, Qt::AscendingOrder);
}

void ChatRoomMembers::submit()
{
    QList<QXmppMucAdminIq::Item> items;
    for (int i = 0; i < m_tableWidget->rowCount(); i++) {
        const QComboBox *combo = qobject_cast<QComboBox *>(m_tableWidget->cellWidget(i, AffiliationColumn));
        Q_ASSERT(m_tableWidget->item(i, JidColumn) && combo);

        QXmppMucAdminIq::Item item;
        item.setAffiliation(static_cast<QXmppMucAdminIq::Item::Affiliation>(combo->itemData(combo->currentIndex()).toInt()));
        item.setJid(m_tableWidget->item(i, JidColumn)->text());
        items << item;
    }

    m_room->setPermissions(items);
    accept();
}

void ChatRoomMembers::addMember()
{
    bool ok = false;
    QString jid = QInputDialog::getText(this, tr("Add a user"),
                  tr("Enter the address of the user you want to add."),
                  QLineEdit::Normal, m_defaultJid, &ok).toLower();
    if (ok)
        addEntry(jid, QXmppMucAdminIq::Item::MemberAffiliation);
}

void ChatRoomMembers::addEntry(const QString &jid, QXmppMucAdminIq::Item::Affiliation affiliation)
{
    QComboBox *combo = new QComboBox;
    combo->addItem(tr("member"), QXmppMucAdminIq::Item::MemberAffiliation);
    combo->addItem(tr("administrator"), QXmppMucAdminIq::Item::AdminAffiliation);
    combo->addItem(tr("owner"), QXmppMucAdminIq::Item::OwnerAffiliation);
    combo->addItem(tr("banned"), QXmppMucAdminIq::Item::OutcastAffiliation);
    combo->setEditable(false);
    combo->setCurrentIndex(combo->findData(affiliation));
    QTableWidgetItem *jidItem = new QTableWidgetItem(jid);
    jidItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_tableWidget->insertRow(0);
    m_tableWidget->setCellWidget(0, AffiliationColumn, combo);
    m_tableWidget->setItem(0, JidColumn, jidItem);
}

void ChatRoomMembers::removeMember()
{
    m_tableWidget->removeRow(m_tableWidget->currentRow());
}

// PLUGIN

class RoomsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Multi-user chat"; };
};

bool RoomsPlugin::initialize(Chat *chat)
{
    new ChatRoomWatcher(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(rooms, RoomsPlugin)

