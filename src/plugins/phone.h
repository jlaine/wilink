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
class QDeclarativeView;
class QNetworkAccessManager;
class Chat;
class ChatClient;
class PhoneCallsModel;
class SipCall;
class SipClient;

class PhonePanel : public ChatPanel
{
    Q_OBJECT

public:
    PhonePanel(Chat *chatWindow, QWidget *parent = NULL);
    ~PhonePanel();

private slots:
    void callButtonClicked(QAbstractButton *button);
    void callReceived(SipCall *call);
    void handleSettings();
    void openUrl(const QUrl &url);

private:
    QAction *action;
    ChatClient *client;
    Chat *m_window;
    SipClient *sip;
    QNetworkAccessManager *network;

    PhoneCallsModel *callsModel;
    QDeclarativeView *declarativeView;

    bool m_registeredHandler;
};

#endif
