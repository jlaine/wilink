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
#include <QDeclarativeItem>
#include <QWidget>

#include "QXmppCallManager.h"
#include "QXmppLogger.h"

#include "chat_panel.h"

class CallVideoItem;
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
class QSoundMeter;
class QTimer;
class QVideoGrabber;
class QXmppCall;
class QXmppCallManager;
class QXmppVideoFormat;
class QXmppVideoFrame;
class QXmppSrvInfo;

class CallAudioHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QXmppCall* call READ call WRITE setCall NOTIFY callChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)

public:
    CallAudioHelper(QObject *parent = 0);

    QXmppCall *call() const;
    void setCall(QXmppCall *call);

    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;

signals:
    void callChanged(QXmppCall *call);

    // This signal is emitted when the input volume changes.
    void inputVolumeChanged(int volume);

    // This signal is emitted when the output volume changes.
    void outputVolumeChanged(int volume);

private slots:
    void audioModeChanged(QIODevice::OpenMode mode);

private:
    QXmppCall *m_call;
    QAudioInput *m_audioInput;
    QSoundMeter *m_audioInputMeter;
    QAudioOutput *m_audioOutput;
    QSoundMeter *m_audioOutputMeter;
};

class CallVideoHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QXmppCall* call READ call WRITE setCall NOTIFY callChanged)

public:
    CallVideoHelper(QObject *parent = 0);

    QXmppCall *call() const;
    void setCall(QXmppCall *call);

signals:
    void callChanged(QXmppCall *call);

private slots:
    void videoModeChanged(QIODevice::OpenMode mode);
    void videoCapture(const QXmppVideoFrame &frame);
    void videoRefresh();

private:
    QXmppCall *m_call;
    QXmppVideoFrame *m_videoConversion;
    QVideoGrabber *m_videoGrabber;
    CallVideoItem *m_videoOutput;
    CallVideoItem *m_videoMonitor;
    QTimer *m_videoTimer;
};

class CallVideoItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)

public:
    CallVideoItem(QDeclarativeItem *parent = 0);

    qreal radius() const;
    void setRadius(qreal radius);

    QRectF boundingRect() const;
    void present(const QXmppVideoFrame &frame);
    void setFormat(const QXmppVideoFormat &format);

signals:
    void radiusChanged(qreal radius);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

private:
    QImage m_image;
    qreal m_radius;
};

class CallVideoWidget : public QGraphicsItem
{
public:
    CallVideoWidget(QGraphicsItem *parent = 0);
    QRectF boundingRect() const;
    void present(const QXmppVideoFrame &frame);
    void setFormat(const QXmppVideoFormat &format);
    QSizeF size() const;
    void setSize(const QSizeF &size);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

private:
    QRectF m_boundingRect;
    QImage m_image;
};

class CallWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent = 0);
    ~CallWidget();
    void setGeometry(const QRectF &rect);

signals:
    void finished();

private slots:
    void callRinging();
    void callStateChanged(QXmppCall::State state);
    void videoModeChanged(QIODevice::OpenMode mode);
    void videoCapture(const QXmppVideoFrame &frame);
    void videoRefresh();

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    void debug(const QString&) {};
    void warning(const QString&) {};
    void setStatus(const QString &status);

    // audio
    ChatPanelProgress *m_audioInputBar;
    QGraphicsPixmapItem *m_audioInputIcon;
    ChatPanelProgress *m_audioOutputBar;
    QGraphicsPixmapItem *m_audioOutputIcon;

    // video
    ChatPanelButton *m_videoButton;
    QXmppVideoFrame *m_videoConversion;
    QVideoGrabber *m_videoGrabber;
    CallVideoWidget *m_videoOutput;
    CallVideoWidget *m_videoMonitor;
    QTimer *m_videoTimer;

    QList<ChatPanelButton*> m_buttons;
    ChatPanelButton *m_hangupButton;
    ChatPanelText *m_label;
    QXmppCall *m_call;
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
