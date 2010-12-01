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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include "chat_panel.h"

#include "plugins/phone/sip.h"

class QAbstractButton;
class QAuthenticator;
class QGraphicsSimpleTextItem;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class Chat;
class ChatClient;
class SipCall;
class SipClient;

class PhoneWidget : public ChatPanelWidget
{
    Q_OBJECT

public:
    PhoneWidget(SipCall *call, QGraphicsItem *parent = 0);
    void setGeometry(const QRectF &rect);

private slots:
    void callFinished();
    void callRinging();
    void callStateChanged(QXmppCall::State state);

private:
    SipCall *m_call;
    QGraphicsSimpleTextItem *m_nameLabel;
    QGraphicsSimpleTextItem *m_statusLabel;
};

class PhonePanel : public ChatPanel
{
    Q_OBJECT

public:
    PhonePanel(Chat *chatWindow, QWidget *parent = NULL);
    void addWidget(ChatPanelWidget *widget);

private slots:
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void backspacePressed();
    void callClicked(QAbstractButton *button);
    void callNumber();
    void callReceived(SipCall *call);
    void getSettings();
    void handleSettings();
    void keyPressed();
    void stateChanged(SipClient::State state);

private:
    ChatClient *client;
    Chat *m_window;
    SipClient *sip;
    QNetworkAccessManager *network;

    QPushButton *callButton;
    ChatPanelBar *callBar;
    QLineEdit *numberEdit;
    QLabel *statusLabel;
};

#endif
