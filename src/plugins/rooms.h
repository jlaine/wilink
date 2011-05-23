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
#include "chat_model.h"
#include "chat_panel.h"

class Chat;
class ChatClient;
class ChatHistoryModel;
class ChatMessage;
class RoomListModel;
class RoomPanel;
class ChatRosterModel;
class QDeclarativeView;
class QAbstractButton;
class QModelIndex;
class QTableWidget;
class QXmppBookmarkManager;
class QXmppBookmarkSet;
class QXmppClient;
class QXmppDataForm;
class QXmppIq;
class QXmppMessage;
class QXmppMucAdminIq;
class QXmppMucManager;
class QXmppMucRoom;
class QXmppPresence;

class ChatRoomModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(QXmppMucRoom* room READ room WRITE setRoom NOTIFY roomChanged)
    Q_PROPERTY(ChatHistoryModel* historyModel READ historyModel CONSTANT)

public:
    ChatRoomModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    ChatHistoryModel *historyModel() const;

    QXmppMucRoom *room() const;
    void setRoom(QXmppMucRoom *room);

signals:
    void roomChanged(QXmppMucRoom *room);

private slots:
    void messageReceived(const QXmppMessage &msg);
    void participantAdded(const QString &jid);
    void participantChanged(const QString &jid);
    void participantRemoved(const QString &jid);

private:
    ChatHistoryModel *m_historyModel;
    QXmppMucRoom *m_room;
};

class ChatRoomWatcher : public QObject
{
    Q_OBJECT

public:
    ChatRoomWatcher(Chat *chatWindow);
    RoomPanel *joinRoom(const QString &jid, bool focus);

private slots:
    void bookmarksReceived();
    void invitationReceived(const QString &roomJid, const QString &jid, const QString &text);
    void invitationHandled(QAbstractButton *button);
    void urlClick(const QUrl &url);

private:
    Chat *chat;
    RoomListModel *roomModel;
    QXmppMucManager *mucManager;
    QStringList invitations;
};

class RoomPanel : public ChatPanel
{
    Q_OBJECT

public:
    RoomPanel(Chat *chatWindow, const QString &jid);

public slots:
    void bookmark();
    void unbookmark();

private slots:
    void changePermissions();
    void configurationReceived(const QXmppDataForm &form);
    void left();

private:
    Chat *chat;
    QXmppMucRoom *mucRoom;

    QDeclarativeView *historyView;
};

class RoomPermissionDialog : public QDialog
{
    Q_OBJECT

public:
    RoomPermissionDialog(QXmppMucRoom *mucRoom, const QString &defaultJid, QWidget *parent);

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

#endif
