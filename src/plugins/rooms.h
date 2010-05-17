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

#ifndef __WILINK_ROOMS_H__
#define __WILINK_ROOMS_H__

#include <QDialog>

#include "chat_conversation.h"

class Chat;
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
class QXmppClient;
class QXmppDiscoveryIq;
class QXmppIq;
class QXmppMessage;
class QXmppMucOwnerIq;
class QXmppPresence;

class ChatRoomWatcher : public QObject
{
    Q_OBJECT

public:
    ChatRoomWatcher(Chat *chatWindow);

private slots:
    void disconnected();
    void inviteContact();
    ChatRoom *joinRoom(const QString &jid);
    void kickUser();
    void messageHandled(QAbstractButton *button);
    void messageReceived(const QXmppMessage &msg);
    void mucOwnerIqReceived(const QXmppMucOwnerIq &iq);
    void mucServerFound(const QString &roomServer);
    void roomJoin();
    void roomOptions();
    void roomMembers();
    void rosterDrop(QDropEvent *event, const QModelIndex &index);
    void rosterMenu(QMenu *menu, const QModelIndex &index);

private:
    Chat *chat;
    QString chatRoomServer;
    QPushButton *roomButton;
    QStringList invitations;
};

class ChatRoom : public ChatConversation
{
    Q_OBJECT

public:
    ChatRoom(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);
    ChatRosterItem::Type objectType() const;
    void invite(const QString &jid);

protected:
    virtual void sendMessage(const QString &text);

private slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void join();
    void leave();
    void disconnected();
    void messageReceived(const QXmppMessage &msg);
    void presenceReceived(const QXmppPresence &msg);
    void rosterClick(const QModelIndex &index);
    void tabPressed();

private:
    QString chatLocalJid;
    QXmppClient *client;
    bool joined;
    bool notifyMessages;
    ChatRosterModel *rosterModel;
};

class ChatRoomMembers : public QDialog
{
    Q_OBJECT

public:
    ChatRoomMembers(QXmppClient *client, const QString &roomJid, QWidget *parent);

protected slots:
    void addMember();
    void removeMember();
    void iqReceived(const QXmppIq &iq);
    void submit();

private:
    void addEntry(const QString &jid, const QString &affiliation);
    QString chatRoomJid;
    QXmppClient *client;
    QXmppElement form;
    QTableWidget *tableWidget;
    QMap<QString, QString> initialMembers;
    QMap<QString, QString> affiliations;
};

class ChatRoomPrompt : public QDialog
{
    Q_OBJECT

public:
    ChatRoomPrompt(QXmppClient *client, const QString &roomServer, QWidget *parent = 0);
    QString textValue() const;

protected slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void itemClicked(QListWidgetItem * item);
    void validate();

private:
    QLineEdit *lineEdit;
    QListWidget *listWidget;
    QString chatRoomServer;
};

#endif
