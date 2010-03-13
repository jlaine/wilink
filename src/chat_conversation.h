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

#ifndef __WDESKTOP_CHAT_CONVERSATION_H__
#define __WDESKTOP_CHAT_CONVERSATION_H__

#include "qxmpp/QXmppMessage.h"

#include "chat_panel.h"

class ChatEdit;
class ChatHistory;
class QLabel;
class QLineEdit;
class QXmppVCard;

class ChatConversation : public ChatPanel
{
    Q_OBJECT

public:
    ChatConversation(const QString &jid, QWidget *parent = NULL);

    virtual void join();
    virtual void leave();

    QXmppMessage::State localState() const;
    void setLocalName(const QString &name);
    void setRemoteName(const QString &name);
    void setRemoteState(QXmppMessage::State state);

protected slots:
    void slotFocused();
    void slotInactive();
    void slotNewLine();
    void slotPaused();
    void slotSend();
    void slotTextChanged();

signals:
    void localStateChanged(QXmppMessage::State state);

protected:
    virtual void sendMessage(const QString &body) = 0;

protected:
    ChatHistory *chatHistory;
    ChatEdit *chatInput;

    QString chatLocalName;
    QString chatRemoteJid;
    QString chatRemoteName;

private:
    QXmppMessage::State chatLocalState;
    QTimer *inactiveTimer;
    QTimer *pausedTimer;
};

#endif
