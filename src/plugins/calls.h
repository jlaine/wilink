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

#ifndef __WILINK_CALLS_H__
#define __WILINK_CALLS_H__

#include <QIODevice>
#include <QObject>
#include <QAudio>

#include "chat_panel.h"

class Chat;
class ChatClient;
class ChatRosterModel;
class QAbstractButton;
class QAudioFormat;
class QAudioInput;
class QAudioOutput;
class QMenu;
class QModelIndex;
class QTimer;
class QXmppCall;

class Generator : public QObject
{
    Q_OBJECT

public:
    Generator(const QAudioFormat &format, qint64 durationUs, int frequency, QObject *parent);
    ~Generator();

    void start(QIODevice *device);
    void stop();

private:
    void generateData(const QAudioFormat &format, qint64 durationUs, int frequency);

private slots:
    void tick();

private:
    QByteArray m_buffer;
    QTimer *m_timer;
    QIODevice *m_device;
};

class CallPanel : public ChatPanel
{
    Q_OBJECT

public:
    CallPanel(QXmppCall *call, QWidget *parent = 0);

private slots:
    void callBuffered();
    void callConnected();
    void stateChanged(QAudio::State state);

private:
    QXmppCall *m_call;
};

class CallWatcher : public QObject
{
    Q_OBJECT

public:
    CallWatcher(Chat *chatWindow);

private slots:
    void callClicked(QAbstractButton * button);
    void callContact();
    void callReceived(QXmppCall *call);
    void rosterMenu(QMenu *menu, const QModelIndex &index);

private:
    ChatClient *m_client;
    ChatRosterModel *m_roster;
    Chat *m_window;
};

#endif
