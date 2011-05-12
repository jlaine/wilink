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

#ifndef __WILINK_CHAT_CONVERSATION_H__
#define __WILINK_CHAT_CONVERSATION_H__

#include <QDeclarativeView>

#include "QXmppMessage.h"

#include "chat_panel.h"

class ChatConversationPrivate;
class ChatEdit;
class ChatHistoryModel;
class ChatHistoryWidget;
class ChatMessage;
class ChatRosterModel;
class QListView;
class QModelIndex;
class QSplitter;

class ChatConversation : public ChatPanel
{
    Q_OBJECT

public:
    ChatConversation(QWidget *parent = NULL);
    ~ChatConversation();
    ChatHistoryModel *historyModel();
    QDeclarativeView *historyView();
    ChatRosterModel *rosterModel();
    void setRosterModel(ChatRosterModel *rosterModel);

signals:
    void messageClicked(const QModelIndex &index);

protected slots:
    void slotSearchDisplayed(bool visible);

protected:
    ChatEdit *chatInput();
    QSplitter *splitter();

private:
    ChatConversationPrivate *d;
};

#endif
