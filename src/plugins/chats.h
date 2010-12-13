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

#ifndef __WILINK_CHAT_DIALOG_H__
#define __WILINK_CHAT_DIALOG_H__

#include <QWidget>

#include "QXmppMessage.h"

#include "chat_conversation.h"

class Chat;
class ChatClient;
class ChatRosterModel;
class QModelIndex;
class QXmppArchiveChat;
class QXmppArchiveManager;

class ChatDialog : public ChatConversation
{
    Q_OBJECT

public:
    ChatDialog(ChatClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent = NULL);
    ChatRosterItem::Type objectType() const;

public slots:
    void messageReceived(const QXmppMessage &msg);

private slots:
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats);
    void chatStateChanged(QXmppMessage::State state);
    void disconnected();
    void join();
    void leave();
    void returnPressed();

private:
    QXmppArchiveManager *archiveManager;
    QString chatRemoteJid;
    ChatClient *client;
    bool joined;
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
