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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include "chat_panel.h"

#include "plugins/phone/sip.h"

class QAbstractButton;
class QAuthenticator;
class QGraphicsSimpleTextItem;
class QLabel;
class QLineEdit;
class QModelIndex;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QThread;
class Chat;
class ChatClient;
class PhoneCallsModel;
class PhoneCallsView;
class SipCall;
class SipClient;

class PhonePanel : public ChatPanel
{
    Q_OBJECT

public:
    PhonePanel(Chat *chatWindow, QWidget *parent = NULL);
    ~PhonePanel();

private slots:
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void backspacePressed();
    void callButtonClicked(QAbstractButton *button);
    void callNumber();
    void callReceived(SipCall *call);
    void callStateChanged(bool haveCalls);
    void error(const QString &error);
    void getSettings();
    void handleSettings();
    void historyClicked(const QModelIndex &index);
    void historyDoubleClicked(const QModelIndex &index);
    void keyPressed();
    void keyReleased();
    void openUrl(const QUrl &url);
    void sipStateChanged(SipClient::State state);

private:
    QAction *action;
    ChatClient *client;
    Chat *m_window;
    SipClient *sip;
    QNetworkAccessManager *network;

    PhoneCallsModel *callsModel;
    PhoneCallsView *callsView;

    QPushButton *callButton;
    QPushButton *hangupButton;
    QLineEdit *numberEdit;

    bool m_registeredHandler;
};

#endif
