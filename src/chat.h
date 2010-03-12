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

class ChatConsole;
class ChatConversation;
class ChatRoom;
class ChatRosterModel;
class ChatRosterView;
class ChatShares;
class ChatTransfers;
class QComboBox;
class QLabel;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QSystemTrayIcon;
class QXmppArchiveChat;
class QXmppShareIq;
class QXmppTransferJob;
class QXmppVCard;
class QXmppVCardManager;

void dumpElement(const QXmppElement &item, int level = 0);

class ChatClient : public QXmppClient
{
    Q_OBJECT

public:
    ChatClient(QObject *parent);
    virtual bool handleStreamElement(const QDomElement &element);

signals:
    void shareIqReceived(const QXmppShareIq &iq);
};

class Chat : public QWidget
{
    Q_OBJECT

public:
    Chat(QSystemTrayIcon *trayIcon);
    ~Chat();
    bool open(const QString &jid, const QString &password, bool ignoreSslErrors);

protected slots:
    void addContact();
    void addPanel(QWidget *panel);
    void addRoom();
    void connected();
    void disconnected();
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void error(QXmppClient::Error error);
    void inviteContact(const QString &jid);
    void iqReceived(const QXmppIq&);
    void joinConversation(const QString &jid, bool isRoom);
    void leaveConversation();
    void messageReceived(const QXmppMessage &msg);
    void panelChanged(int index);
    void presenceReceived(const QXmppPresence &presence);
    void rejoinConversations();
    void removePanel(QWidget *panel);
    void rosterAction(int action, const QString &jid, int type);
    void removeContact(const QString &jid);
    void resizeContacts();
    void statusChanged(int currentIndex);

    void hideConsole();
    void showConsole();

    void hideTransfers();
    void showTransfers();

protected:
    void changeEvent(QEvent *event);
    ChatConversation *createConversation(const QString &jid, bool room);

private:
    bool isBusy;
    bool isConnected;

    QPushButton *addButton;
    QPushButton *roomButton;
    QPushButton *sharesButton;
    ChatConsole *chatConsole;
    ChatShares *chatShares;
    ChatTransfers *chatTransfers;
    ChatClient *client;

    ChatRosterModel *rosterModel;
    ChatRosterView *rosterView;

    QSplitter *splitter;
    QComboBox *statusCombo;
    QSystemTrayIcon *systemTrayIcon;
    QStackedWidget *conversationPanel;

    QString chatRoomServer;
    QStringList discoQueue;
};


#endif
