/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_H__
#define __WDESKTOP_CHAT_H__

#include <QWidget>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class ChatDialog;
class ChatRoom;
class ChatRoomsModel;
class ChatRoomsView;
class RosterModel;
class RosterView;
class QLabel;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QSystemTrayIcon;
class QXmppArchiveChat;
class QXmppVCard;
class QXmppVCardManager;

class Chat : public QWidget
{
    Q_OBJECT

public:
    Chat(QSystemTrayIcon *trayIcon);
    bool open(const QString &jid, const QString &password);

protected slots:
    void addContact();
    void addRoom();
    void chatContact(const QString &jid);
    void chatRoom(const QString &jid);
    void connected();
    void disconnected();
    void error(QXmppClient::Error error);
    void discoveryIqReceived(const QXmppDiscoveryIq&);
    void iqReceived(const QXmppIq&);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats);
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void messageReceived(const QXmppMessage &msg);
    void presenceReceived(const QXmppPresence &presence);
    void reconnect();
    void removeContact(const QString &jid);
    void resizeContacts();
    void sendMessage(const QXmppMessage &msg);
    void sendPing();
    void vCardReceived(const QXmppVCard&);

protected:
    void changeEvent(QEvent *event);
    ChatDialog *conversation(const QString &jid);
    ChatRoom *room(const QString &jid);

private:
    bool reconnectOnDisconnect;

    QPushButton *addButton;
    QPushButton *roomButton;
    QHash<QString, ChatDialog*> chatDialogs;
    QHash<QString, ChatRoom*> chatRooms;
    QXmppClient *client;
    QTimer *pingTimer;

    RosterModel *rosterModel;
    RosterView *rosterView;

    ChatRoomsModel *roomsModel;
    ChatRoomsView *roomsView;

    QSplitter *splitter;
    QLabel *statusIconLabel;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QTimer *timeoutTimer;
    QStackedWidget *conversationPanel;

    QString ownName;
};


#endif
