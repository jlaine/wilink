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
#include <QGraphicsPixmapItem>
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
class QGraphicsLinearLayout;
class QHostInfo;
class QLabel;
class QMenu;
class QModelIndex;
class QTimer;
class QVideoGrabber;
class QXmppCall;
class QXmppCallManager;
class QXmppVideoFormat;
class QXmppVideoFrame;
class QXmppSrvInfo;

class CallVideoWidget : public QGraphicsPixmapItem, public QGraphicsLayoutItem
{
public:
    CallVideoWidget(QGraphicsItem *parent = 0);
    void present(const QXmppVideoFrame &frame);
    void setFormat(const QXmppVideoFormat &format);

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    QImage m_videoImage;
};

class CallWidget : public ChatPanelWidget
{
    Q_OBJECT

public:
    CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent = 0);
    ~CallWidget();

private slots:
    void audioModeChanged(QIODevice::OpenMode mode);
    void audioStateChanged(QAudio::State state);
    void callRinging();
    void callStateChanged(QXmppCall::State state);
    void videoModeChanged(QIODevice::OpenMode mode);
    void videoCapture();
    void videoRefresh();

private:
    void debug(const QString&) {};
    void warning(const QString&) {};

    // audio
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;

    // video
    QVideoGrabber *m_videoGrabber;
    QTimer *m_videoTimer;
    CallVideoWidget *m_videoOutput;

    ChatPanelButton *m_button;
    QXmppCall *m_call;
    ChatPanelText *m_label;
    QGraphicsLinearLayout *m_layout;
    int m_soundId;
};

class CallWatcher : public QXmppLoggable
{
    Q_OBJECT

public:
    CallWatcher(Chat *chatWindow);
    ~CallWatcher();

private slots:
    void callAborted();
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
    QMap<QXmppCall*, int> m_callQueue;
    QXmppClient *m_client;
    quint16 m_turnPort;
    Chat *m_window;
};

#endif
