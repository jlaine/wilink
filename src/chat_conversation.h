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

#ifndef __WILINK_CHAT_CONVERSATION_H__
#define __WILINK_CHAT_CONVERSATION_H__

#include "qxmpp/QXmppMessage.h"

#include "chat_panel.h"

class ChatEdit;
class ChatHistory;
class ChatSearchBar;
class QLabel;
class QSpacerItem;
class QLineEdit;
class QXmppVCard;

class ChatConversation : public ChatPanel
{
    Q_OBJECT

public:
    ChatConversation(const QString &jid, QWidget *parent = NULL);

    QXmppMessage::State localState() const;

protected slots:
    void slotFocused();
    void slotInactive();
    void slotPaused();
    void slotSend();
    void slotSearchDisplayed(bool visible);
    void slotTextChanged();

signals:
    void localStateChanged(QXmppMessage::State state);

protected:
    virtual bool sendMessage(const QString &body) = 0;
    void setRemoteState(QXmppMessage::State state);

protected:
    ChatHistory *chatHistory;
    ChatEdit *chatInput;
    ChatSearchBar *chatSearch;

    QString chatRemoteJid;

private:
    QSpacerItem *spacerItem;
    QXmppMessage::State chatLocalState;
    QTimer *inactiveTimer;
    QTimer *pausedTimer;
};

#endif
