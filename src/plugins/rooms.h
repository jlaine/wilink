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
#include "chat_conversation.h"

class Chat;
class ChatClient;
class ChatMessage;
class ChatRoom;
class ChatRosterModel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QAbstractButton;
class QMenu;
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
    void kickUser();
    void invitationReceived(const QString &roomJid, const QString &jid, const QString &text);
    void invitationHandled(QAbstractButton *button);
    void mucServerFound(const QString &roomServer);
    void roomClose();
    void roomPrompt();
    void rosterClick(const QModelIndex &index);
    void rosterDrop(QDropEvent *event, const QModelIndex &index);
    void rosterMenu(QMenu *menu, const QModelIndex &index);
    void urlClick(const QUrl &url);

private:
    bool bookmarkRoom(const QString &jid);
    bool unbookmarkRoom(const QString &jid);

    Chat *chat;
    QXmppBookmarkManager *bookmarkManager;
    QString chatRoomServer;
    QPushButton *roomButton;
    QStringList invitations;
};

class ChatRoom : public ChatConversation
{
    Q_OBJECT

public:
    ChatRoom(Chat *chatWindow, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);
    ChatRosterModel::Type objectType() const;
    void invite(const QString &jid);

private slots:
    void changeSubject();
    void changePermissions();
    void configurationReceived(const QXmppDataForm &form);
    void discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void disconnected();
    void join();
    void joined();
    void leave();
    void onMessageClicked(const QModelIndex &messageIndex);
    void messageReceived(const QXmppMessage &msg);
    void presenceReceived(const QXmppPresence &presence);
    void returnPressed();
    void rosterClick(const QModelIndex &index);
    void subjectChanged(const QString &subject);
    void tabPressed();

private:
    Chat *chat;
    QXmppMucRoom *mucRoom;
    QString nickName;
    bool notifyMessages;
    ChatRosterModel *rosterModel;

    // actions
    QAction *subjectAction;
    QAction *optionsAction;
    QAction *permissionsAction;
};

class ChatRoomMembers : public QDialog
{
    Q_OBJECT

public:
    ChatRoomMembers(QXmppMucRoom *mucRoom, const QString &defaultJid, QWidget *parent);

protected slots:
    void addMember();
    void removeMember();
    void permissionsReceived(const QList<QXmppMucAdminIq::Item> &permissions);
    void submit();

private:
    void addEntry(const QString &jid, QXmppMucAdminIq::Item::Affiliation affiliation);

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
