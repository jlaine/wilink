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

#ifndef __WILINK_CALLS_H__
#define __WILINK_CALLS_H__

#include <QAudioInput>
#include <QObject>
#include <QWidget>

#include "QXmppCallManager.h"
#include "QXmppLogger.h"

#include "chat_panel.h"

class Chat;
class ChatRosterModel;
class QAbstractButton;
class QAudioInput;
class QAudioOutput;
class QFile;
class QHostInfo;
class QLabel;
class QMenu;
class QModelIndex;
class QTimer;
class QXmppCall;
class QXmppCallManager;
class QXmppSrvInfo;

class CallWidget : public ChatPanelWidget
{
    Q_OBJECT

public:
    CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent = 0);
    ~CallWidget();
    void setGeometry(const QRectF &rect);

private slots:
    void audioStateChanged(QAudio::State state);
    void callRinging();
    void callStateChanged(QXmppCall::State state);

private:
    void debug(const QString&) {};
    void warning(const QString&) {};
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;

    QXmppCall *m_call;
    QGraphicsSimpleTextItem *m_label;
    int m_soundId;
};

class CallWatcher : public QXmppLoggable
{
    Q_OBJECT

public:
    CallWatcher(Chat *chatWindow);
    ~CallWatcher();

private slots:
    void callClicked(QAbstractButton * button);
    void callContact();
    void callReceived(QXmppCall *call);
    void connected();
    void rosterMenu(QMenu *menu, const QModelIndex &index);
    void setTurnServer(const QXmppSrvInfo &serviceInfo);
    void setTurnServer(const QHostInfo &hostInfo);

private:
    void addCall(QXmppCall *call);

    QXmppCallManager *m_callManager;
    QXmppClient *m_client;
    quint16 m_turnPort;
    Chat *m_window;
};

#endif
