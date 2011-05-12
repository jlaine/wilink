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

#ifndef __WILINK_ROOMS_H__
#define __WILINK_ROOMS_H__

#include <QDialog>

#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "chat_conversation.h"

class Chat;
class ChatClient;
class ChatMessage;
class ChatRoom;
class ChatRosterModel;
class RoomInviteModel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QAbstractButton;
class QModelIndex;
class QPushButton;
class QTableWidget;
class QXmppBookmarkManager;
class QXmppBookmarkSet;
class QXmppClient;
class QXmppDataForm;
class QXmppDiscoveryIq;
class QXmppIq;
class QXmppMessage;
class QXmppMucAdminIq;
class QXmppMucManager;
class QXmppMucRoom;
class QXmppPresence;

class ChatRoomWatcher : public QObject
{
    Q_OBJECT

public:
    ChatRoomWatcher(Chat *chatWindow);
    ChatRoom *joinRoom(const QString &jid, bool focus);

private slots:
    void bookmarksReceived();
    void disconnected();
    void invitationReceived(const QString &roomJid, const QString &jid, const QString &text);
    void invitationHandled(QAbstractButton *button);
    void mucServerFound(const QString &roomServer);
    void roomPrompt();
    void rosterClick(const QModelIndex &index);
    void urlClick(const QUrl &url);

private:
    Chat *chat;
    QXmppMucManager *mucManager;
    QString chatRoomServer;
    QPushButton *roomButton;
    QStringList invitations;
};

class ChatRoom : public ChatConversation
{
    Q_OBJECT

public:
    ChatRoom(Chat *chatWindow, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);
    void invite(const QString &jid);

public slots:
    void bookmark();
    void unbookmark();

private slots:
    void allowedActionsChanged(QXmppMucRoom::Actions actions);
    void changeSubject();
    void changePermissions();
    void configurationReceived(const QXmppDataForm &form);
    void customContextMenuRequested(const QPoint & pos);
    void discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void error(const QXmppStanza::Error &error);
    void inviteDialog();
    void join();
    void left();
    void kicked(const QString &jid, const QString &reason);
    void kickUser();
    void onMessageClicked(const QModelIndex &messageIndex);
    void messageReceived(const QXmppMessage &msg);
    void participantAdded(const QString &jid);
    void participantChanged(const QString &jid);
    void participantClicked(const QModelIndex &index);
    void participantRemoved(const QString &jid);
    void returnPressed();
    void showProfile();
    void subjectChanged(const QString &subject);

private:
    Chat *chat;
    QXmppMucRoom *mucRoom;
    bool notifyMessages;
    ChatRosterModel *rosterModel;
    QListView *participantsList;

    // actions
    QAction *subjectAction;
    QAction *optionsAction;
    QAction *permissionsAction;
};

class ChatRoomInvite : public QDialog
{
    Q_OBJECT

public:
    ChatRoomInvite(QXmppMucRoom *mucRoom, ChatRosterModel *rosterModel, QWidget *parent);

protected slots:
    void submit();

private:
    QListView *m_list;
    RoomInviteModel *m_model;
    QLineEdit *m_reason;
    QXmppMucRoom *m_room;
};

class ChatRoomMembers : public QDialog
{
    Q_OBJECT

public:
    ChatRoomMembers(QXmppMucRoom *mucRoom, const QString &defaultJid, QWidget *parent);

protected slots:
    void addMember();
    void removeMember();
    void permissionsReceived(const QList<QXmppMucItem> &permissions);
    void submit();

private:
    void addEntry(const QString &jid, QXmppMucItem::Affiliation affiliation);

    QString m_defaultJid;
    QXmppMucRoom *m_room;
    QTableWidget *m_tableWidget;
};

class ChatRoomPrompt : public QDialog
{
    Q_OBJECT

public:
    ChatRoomPrompt(QXmppClient *client, const QString &roomServer, QWidget *parent = 0);
    QString textValue() const;

protected slots:
    void discoveryItemsReceived(const QXmppDiscoveryIq &disco);
    void itemClicked(QListWidgetItem * item);
    void validate();

private:
    QLineEdit *lineEdit;
    QListWidget *listWidget;
    QString chatRoomServer;
};

#endif
