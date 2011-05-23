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
class QUrl;
class QXmppArchiveChat;
class QXmppArchiveManager;

class ChatDialogHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(ChatHistoryModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(ChatRosterModel* rosterModel READ rosterModel WRITE setRosterModel NOTIFY rosterModelChanged)
    Q_PROPERTY(int localState READ localState WRITE setLocalState NOTIFY localStateChanged)
    Q_PROPERTY(int remoteState READ remoteState NOTIFY remoteStateChanged)

public:
    ChatDialogHelper(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    ChatHistoryModel *historyModel() const;

    QString jid() const;
    void setJid(const QString &jid);

    int localState() const;
    void setLocalState(int state);

    int remoteState() const;

    ChatRosterModel *rosterModel() const;
    void setRosterModel(ChatRosterModel *rosterModel);

signals:
    void clientChanged(ChatClient *client);
    void localStateChanged(int localState);
    void jidChanged(const QString &jid);
    void remoteStateChanged(int remoteState);
    void rosterModelChanged(ChatRosterModel *rosterModel);

public slots:
    bool sendMessage(const QString &text);

private slots:
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats);
    void fetchArchives();
    void messageReceived(const QXmppMessage &msg);

private:
    bool m_archivesFetched;
    ChatClient *m_client;
    ChatHistoryModel *m_historyModel;
    QXmppMessage::State m_localState;
    QString m_jid;
    QXmppMessage::State m_remoteState;
};

class ChatDialogPanel : public ChatPanel
{
    Q_OBJECT

public:
    ChatDialogPanel(Chat *chatWindow, const QString &jid);
};

class ChatsWatcher : public QObject
{
    Q_OBJECT

public:
    ChatsWatcher(Chat *chatWindow);

private slots:
    void messageReceived(const QXmppMessage &msg);
    void urlClick(const QUrl &url);

private:
    Chat *chat;
};

#endif
