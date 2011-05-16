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

#ifndef __WILINK_CHAT_DIALOG_H__
#define __WILINK_CHAT_DIALOG_H__

#include <QWidget>

#include "QXmppMessage.h"

#include "chat_panel.h"

class Chat;
class ChatClient;
class ChatHistoryModel;
class ChatRosterModel;
class QDeclarativeView;
class QModelIndex;
class QXmppArchiveChat;
class QXmppArchiveManager;

class ChatDialogHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(ChatHistoryModel* historyModel READ historyModel WRITE setHistoryModel NOTIFY historyModelChanged)
    Q_PROPERTY(int state READ state WRITE setState NOTIFY stateChanged)

public:
    ChatDialogHelper(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    ChatHistoryModel *historyModel() const;
    void setHistoryModel(ChatHistoryModel *historyModel);

    QString jid() const;
    void setJid(const QString &jid);

    int state() const;
    void setState(int state);
    
signals:
    void clientChanged(ChatClient *client);
    void historyModelChanged(ChatHistoryModel *historyModel);
    void jidChanged(const QString &jid);
    void stateChanged(int state);

public slots:
    bool sendMessage(const QString &text);

private:
    ChatClient *m_client;
    ChatHistoryModel *m_historyModel;
    QString m_jid;
    QXmppMessage::State m_state;
};

class ChatDialog : public ChatPanel
{
    Q_OBJECT

public:
    ChatDialog(ChatClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);
    QDeclarativeView *declarativeView() const;

public slots:
    void messageReceived(const QXmppMessage &msg);
    void rosterChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats);
    void chatStateChanged(int state);
    void disconnected();
    void join();
    void leave();

private:
    void updateWindowTitle();

    QXmppArchiveManager *archiveManager;
    QString chatRemoteJid;
    ChatClient *client;
    bool joined;

    ChatHistoryModel *historyModel;
    QDeclarativeView *historyView;
    ChatRosterModel *rosterModel;
    QStringList chatStatesJids;
};

class ChatsWatcher : public QObject
{
    Q_OBJECT

public:
    ChatsWatcher(Chat *chatWindow);

private slots:
    void messageReceived(const QXmppMessage &msg);
    void rosterClick(const QModelIndex &index);

private:
    Chat *chat;
};

#endif
