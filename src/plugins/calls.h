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

#include <QDeclarativeItem>
#include <QWidget>

#include "QXmppCallManager.h"
#include "QXmppLogger.h"

class CallAudioHelper;
class CallVideoItem;
class QAudioInput;
class QAudioOutput;
class QSoundMeter;
class QTimer;
class QVideoGrabber;
class QXmppRtpAudioChannel;
class QXmppVideoFormat;
class QXmppVideoFrame;

class CallAudioHelperPrivate : public QObject
{
    Q_OBJECT

public:
    CallAudioHelperPrivate(CallAudioHelper *qq);

public slots:
    void audioModeChanged(QIODevice::OpenMode mode);
    void audioStateChanged();

private:
    QAudioInput *m_audioInput;
    QSoundMeter *m_audioInputMeter;
    QAudioOutput *m_audioOutput;
    QSoundMeter *m_audioOutputMeter;
    QXmppRtpAudioChannel *m_channel;
    CallAudioHelper *q;

    friend class CallAudioHelper;
};

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

private:
    QXmppCall *m_call;
    CallAudioHelperPrivate *d;
    friend class CallAudioHelperPrivate;
};

class CallVideoHelper : public QObject
{
    Q_OBJECT
    Q_FLAGS(OpenModeFlag OpenMode)
    Q_PROPERTY(QXmppCall* call READ call WRITE setCall NOTIFY callChanged)
    Q_PROPERTY(CallVideoItem* monitor READ monitor WRITE setMonitor NOTIFY monitorChanged)
    Q_PROPERTY(OpenMode openMode READ openMode NOTIFY openModeChanged)
    Q_PROPERTY(CallVideoItem* output READ output WRITE setOutput NOTIFY outputChanged)

public:
    enum OpenModeFlag {
        NotOpen = 0x0000,
        ReadOnly = 0x0001,
        WriteOnly = 0x0002,
    };
    Q_DECLARE_FLAGS(OpenMode, OpenModeFlag)

    CallVideoHelper(QObject *parent = 0);

    QXmppCall *call() const;
    void setCall(QXmppCall *call);

    CallVideoItem *monitor() const;
    void setMonitor(CallVideoItem *monitor);

    OpenMode openMode() const;

    CallVideoItem *output() const;
    void setOutput(CallVideoItem *output);

signals:
    void callChanged(QXmppCall *call);
    void monitorChanged(CallVideoItem *monitor);
    void openModeChanged();
    void outputChanged(CallVideoItem *output);

private slots:
    void videoModeChanged(QIODevice::OpenMode mode);
    void videoCapture(const QXmppVideoFrame &frame);
    void videoRefresh();

private:
    QXmppCall *m_call;
    QXmppVideoFrame *m_videoConversion;
    QVideoGrabber *m_videoGrabber;
    CallVideoItem *m_videoMonitor;
    CallVideoItem *m_videoOutput;
    QTimer *m_videoTimer;
};

class DeclarativePen : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY penChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY penChanged)

public:
    DeclarativePen(QObject *parent=0)
        : QObject(parent), _width(1), _color("#000000"), _valid(false)
    {}

    int width() const { return _width; }
    void setWidth(int w);

    QColor color() const { return _color; }
    void setColor(const QColor &c);

    bool isValid() { return _valid; }

Q_SIGNALS:
    void penChanged();

private:
    int _width;
    QColor _color;
    bool _valid;
};

class CallVideoItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(DeclarativePen * border READ border CONSTANT)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)

public:
    CallVideoItem(QDeclarativeItem *parent = 0);

    DeclarativePen* border();

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
    DeclarativePen *m_border;
    QImage m_image;
    qreal m_radius;
};

#endif
