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

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QDeclarativeComponent>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QFile>
#include <QHostInfo>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTimer>

#include "QSoundPlayer.h"
#include "QVideoGrabber.h"
#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppJingleIq.h"
#include "QXmppRtpChannel.h"
#include "QXmppSrvInfo.h"
#include "QXmppUtils.h"

#include "QSoundMeter.h"

#include "calls.h"
#include "chats.h"

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"

static QAudioFormat formatFor(const QXmppJinglePayloadType &type)
{
    QAudioFormat format;
    format.setFrequency(type.clockrate());
    format.setChannels(type.channels());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

void DeclarativePen::setColor(const QColor &c)
{
    _color = c;
    _valid = (_color.alpha() && _width >= 1) ? true : false;
    emit penChanged();
}

void DeclarativePen::setWidth(int w)
{
    if (_width == w && _valid)
        return;

    _width = w;
    _valid = (_color.alpha() && _width >= 1) ? true : false;
    emit penChanged();
}

CallAudioHelper::CallAudioHelper(QObject *parent)
    : QObject(parent),
    m_audioInput(0),
    m_audioInputMeter(0),
    m_audioOutput(0),
    m_audioOutputMeter(0),
    m_call(0)
{
}

void CallAudioHelper::audioModeChanged(QIODevice::OpenMode mode)
{
    qDebug("audio mode changed %i", (int)mode);
    QXmppRtpAudioChannel *channel = m_call->audioChannel();
    Q_ASSERT(channel);

    QAudioFormat format = formatFor(channel->payloadType());

#ifdef Q_OS_MAC
    // 128ms at 8kHz
    const int bufferSize = 2048 * format.channels();
#else
    // 160ms at 8kHz
    const int bufferSize = 2560 * format.channels();
#endif

    // start or stop playback
    const bool canRead = (mode & QIODevice::ReadOnly);
    if (canRead && !m_audioOutput) {
        m_audioOutput = new QAudioOutput(wApp->audioOutputDevice(), format, this);
        m_audioOutputMeter = new QSoundMeter(format, channel, this);
        QObject::connect(m_audioOutputMeter, SIGNAL(valueChanged(int)),
                         this, SIGNAL(outputVolumeChanged(int)));
        m_audioOutput->setBufferSize(bufferSize);
        m_audioOutput->start(m_audioOutputMeter);
    } else if (!canRead && m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = 0;
        delete m_audioOutputMeter;
        m_audioOutputMeter = 0;
    }

    // start or stop capture
    const bool canWrite = (mode & QIODevice::WriteOnly);
    if (canWrite && !m_audioInput) {
        m_audioInput = new QAudioInput(wApp->audioInputDevice(), format, this);
        m_audioInputMeter = new QSoundMeter(format, channel, this);
        QObject::connect(m_audioInputMeter, SIGNAL(valueChanged(int)),
                         this, SIGNAL(inputVolumeChanged(int)));
        m_audioInput->setBufferSize(bufferSize);
        m_audioInput->start(m_audioInputMeter);
    } else if (!canWrite && m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = 0;
        delete m_audioInputMeter;
        m_audioInputMeter = 0;
    }
}

QXmppCall* CallAudioHelper::call() const
{
    return m_call;
}

void CallAudioHelper::setCall(QXmppCall *call)
{
    if (call != m_call) {
        m_call = call;

        bool check;
        check = connect(call, SIGNAL(audioModeChanged(QIODevice::OpenMode)),
                        this, SLOT(audioModeChanged(QIODevice::OpenMode)));
        Q_ASSERT(check);

        emit callChanged(call);
    }
}

int CallAudioHelper::inputVolume() const
{
    return m_audioInputMeter ? m_audioInputMeter->value() : 0;
}

int CallAudioHelper::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int CallAudioHelper::outputVolume() const
{
    return m_audioOutputMeter ? m_audioOutputMeter->value() : 0;
}

CallVideoHelper::CallVideoHelper(QObject *parent)
    : QObject(parent),
    m_call(0),
    m_videoConversion(0),
    m_videoGrabber(0),
    m_videoMonitor(0),
    m_videoOutput(0)
{
    bool check;
    m_videoTimer = new QTimer(this);
    check = connect(m_videoTimer, SIGNAL(timeout()),
                    this, SLOT(videoRefresh()));
    Q_ASSERT(check);
}

QXmppCall* CallVideoHelper::call() const
{
    return m_call;
}

void CallVideoHelper::setCall(QXmppCall *call)
{
    if (call != m_call) {
        m_call = call;

        bool check;
        check = connect(call, SIGNAL(videoModeChanged(QIODevice::OpenMode)),
                        this, SLOT(videoModeChanged(QIODevice::OpenMode)));
        Q_ASSERT(check);

        emit callChanged(call);
    }
}

bool CallVideoHelper::enabled() const
{
    return m_call && ((m_call->videoMode() && QIODevice::ReadWrite) != QIODevice::NotOpen);
}

CallVideoItem *CallVideoHelper::output() const
{
    return m_videoOutput;
}

void CallVideoHelper::setOutput(CallVideoItem *output)
{
    if (output != m_videoOutput) {
        m_videoOutput = output;
        emit outputChanged(output);
    }
}

CallVideoItem *CallVideoHelper::monitor() const
{
    return m_videoMonitor;
}

void CallVideoHelper::setMonitor(CallVideoItem *monitor)
{
    if (monitor != m_videoMonitor) {
        m_videoMonitor = monitor;
        emit monitorChanged(monitor);
    }
}

void CallVideoHelper::videoModeChanged(QIODevice::OpenMode mode)
{
    Q_ASSERT(m_call);

    qDebug("video mode changed %i", (int)mode);
    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        mode = QIODevice::NotOpen;

    // start or stop playback
    const bool canRead = (mode & QIODevice::ReadOnly);
    if (canRead && !m_videoTimer->isActive()) {
        if (m_videoOutput) {
            QXmppVideoFormat format = channel->decoderFormat();
            m_videoOutput->setFormat(format);
            m_videoTimer->start(1000 / format.frameRate());
        }
    } else if (!canRead && m_videoTimer->isActive()) {
        m_videoTimer->stop();
    }

    // start or stop capture
    const bool canWrite = (mode & QIODevice::WriteOnly);
    if (canWrite && !m_videoGrabber) {
        const QXmppVideoFormat format = channel->encoderFormat();

        // check we have a video input
        QList<QVideoGrabberInfo> grabbers = QVideoGrabberInfo::availableGrabbers();
        if (grabbers.isEmpty())
            return;

        // determine if we need a conversion
        QList<QXmppVideoFrame::PixelFormat> pixelFormats = grabbers.first().supportedPixelFormats();
        if (!pixelFormats.contains(format.pixelFormat())) {
            qWarning("we need a format conversion");
            QXmppVideoFormat auxFormat = format;
            auxFormat.setPixelFormat(pixelFormats.first());
            m_videoGrabber = new QVideoGrabber(auxFormat);

            QPair<int, int> metrics = QVideoGrabber::byteMetrics(format.pixelFormat(), format.frameSize());
            m_videoConversion = new QXmppVideoFrame(metrics.second, format.frameSize(), metrics.first, format.pixelFormat());
        } else {
            m_videoGrabber = new QVideoGrabber(format);
        }

        connect(m_videoGrabber, SIGNAL(frameAvailable(QXmppVideoFrame)),
                this, SLOT(videoCapture(QXmppVideoFrame)));
        m_videoGrabber->start();

        if (m_videoMonitor)
            m_videoMonitor->setFormat(format);
    } else if (!canWrite && m_videoGrabber) {
        m_videoGrabber->stop();
        delete m_videoGrabber;
        m_videoGrabber = 0;
        if (m_videoConversion) {
            delete m_videoConversion;
            m_videoConversion = 0;
        }
    }

    emit enabledChanged(canRead || canWrite);
}

void CallVideoHelper::videoCapture(const QXmppVideoFrame &frame)
{
    Q_ASSERT(m_call);

    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        return;

    if (frame.isValid()) {
        if (m_videoConversion) {
            //m_videoConversion.setStartTime(frame.startTime());
            QVideoGrabber::convert(frame.size(),
                                   frame.pixelFormat(), frame.bytesPerLine(), frame.bits(),
                                   m_videoConversion->pixelFormat(), m_videoConversion->bytesPerLine(), m_videoConversion->bits());
            channel->writeFrame(*m_videoConversion);
        } else {
            channel->writeFrame(frame);
        }
        if (m_videoMonitor)
            m_videoMonitor->present(frame);
    }
}

void CallVideoHelper::videoRefresh()
{
    Q_ASSERT(m_call);

    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        return;

    QList<QXmppVideoFrame> frames = channel->readFrames();
    if (!frames.isEmpty() && m_videoOutput)
        m_videoOutput->present(frames.last());
}

CallVideoItem::CallVideoItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
    m_radius(8)
{
    m_border = new DeclarativePen(this);
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}

QRectF CallVideoItem::boundingRect() const
{
    return QRectF(0, 0, width(), height());
}

void CallVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const QRectF m_boundingRect(0, 0, width(), height());
    if (m_boundingRect.isEmpty())
        return;
    painter->setPen(QPen(m_border->color(), m_border->width()));
    if (!m_image.size().isEmpty()) {
        QBrush brush(m_image);
        brush.setTransform(brush.transform().scale(m_boundingRect.width() / m_image.width(), m_boundingRect.height() / m_image.height()));
        painter->setBrush(brush);
    } else {
        painter->setBrush(Qt::black);
    }
    painter->drawRoundedRect(m_boundingRect.adjusted(0.5, 0.5, -0.5, -0.5), m_radius, m_radius);
}

void CallVideoItem::present(const QXmppVideoFrame &frame)
{
    if (!frame.isValid() || frame.size() != m_image.size())
        return;
    QVideoGrabber::frameToImage(&frame, &m_image);
    update();
}

void CallVideoItem::setFormat(const QXmppVideoFormat &format)
{
    const QSize size = format.frameSize();
    if (size != m_image.size()) {
        m_image = QImage(size, QImage::Format_RGB32);
        m_image.fill(0);
    }
}

DeclarativePen* CallVideoItem::border()
{
    return m_border;
}

qreal CallVideoItem::radius() const
{
    return m_radius;
}

void CallVideoItem::setRadius(qreal radius)
{
    if (radius != m_radius) {
        m_radius = radius;
        emit radiusChanged(radius);
    }
}

CallWatcher::CallWatcher(Chat *chatWindow)
    : QXmppLoggable(chatWindow), m_window(chatWindow)
{
    m_client = chatWindow->client();

    m_callManager = new QXmppCallManager;
    m_client->addExtension(m_callManager);
    connect(m_callManager, SIGNAL(callReceived(QXmppCall*)),
            this, SLOT(callReceived(QXmppCall*)));
    connect(m_client, SIGNAL(connected()),
            this, SLOT(connected()));
}

CallWatcher::~CallWatcher()
{
}

void CallWatcher::addCall(QXmppCall *call)
{
    const QString bareJid = jidToBareJid(call->jid());
    QModelIndex index = m_window->rosterModel()->findItem(bareJid);
    if (index.isValid())
        QMetaObject::invokeMethod(m_window, "rosterClicked", Q_ARG(QModelIndex, index));

    ChatDialogPanel *panel = qobject_cast<ChatDialogPanel*>(m_window->panel(bareJid));
    if (panel) {
        // load component if needed
        QDeclarativeComponent *component = qobject_cast<QDeclarativeComponent*>(panel->property("__call_component").value<QObject*>());
        if (!component) {
            QDeclarativeEngine *engine = panel->declarativeView()->engine();
            component = new QDeclarativeComponent(engine, QUrl("qrc:/CallWidget.qml"));
            panel->setProperty("__call_component", qVariantFromValue<QObject*>(component));
        }

        // create audio helper
        CallAudioHelper *audioHelper = new CallAudioHelper;
        audioHelper->moveToThread(wApp->soundThread());
        audioHelper->setCall(call);

        // create call widget
        QDeclarativeItem *widget = qobject_cast<QDeclarativeItem*>(component->create());
        Q_ASSERT(widget);
        widget->setProperty("audio", qVariantFromValue<QObject*>(audioHelper));
        widget->setProperty("call", qVariantFromValue<QObject*>(call));
        QDeclarativeItem *bar = panel->declarativeView()->rootObject()->findChild<QDeclarativeItem*>("widgetBar");
        widget->setParentItem(bar);
    }
}

/** The call finished without the user accepting it.
 */
void CallWatcher::callAborted()
{
    QXmppCall *call = qobject_cast<QXmppCall*>(sender());
    if (!call)
        return;

    // stop ring tone
    int soundId = m_callQueue.take(call);
    if (soundId)
        wApp->soundPlayer()->stop(soundId);

    call->deleteLater();
}

void CallWatcher::callClicked(QAbstractButton * button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    QXmppCall *call = qobject_cast<QXmppCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    // stop ring tone
    int soundId = m_callQueue.take(call);
    if (soundId)
        wApp->soundPlayer()->stop(soundId);

    if (box->standardButton(button) == QMessageBox::Yes)
    {
        disconnect(call, SIGNAL(finished()), this, SLOT(callAborted()));
        addCall(call);
        call->accept();
    } else {
        call->hangup();
    }
    box->deleteLater();
}

void CallWatcher::callContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QXmppCall *call = m_callManager->call(fullJid);
    addCall(call);
}

void CallWatcher::callReceived(QXmppCall *call)
{
    const QString contactName = m_window->rosterModel()->contactName(call->jid());

    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Call from %1").arg(contactName),
        tr("%1 wants to talk to you.\n\nDo you accept?").arg(contactName),
        QMessageBox::Yes | QMessageBox::No, m_window);
    box->setDefaultButton(QMessageBox::NoButton);
    box->setProperty("call", qVariantFromValue(qobject_cast<QObject*>(call)));

    // connect signals
    connect(call, SIGNAL(finished()), this, SLOT(callAborted()));
    connect(call, SIGNAL(finished()), box, SLOT(deleteLater()));
    connect(box, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(callClicked(QAbstractButton*)));
    box->show();

    // start incoming tone
    int soundId = wApp->soundPlayer()->play(":/call-incoming.ogg", true);
    m_callQueue.insert(call, soundId);
}

void CallWatcher::connected()
{
    // lookup TURN server
    const QString domain = m_client->configuration().domain();
    debug(QString("Looking up STUN server for domain %1").arg(domain));
    QXmppSrvInfo::lookupService("_turn._udp." + domain, this,
                                SLOT(setTurnServer(QXmppSrvInfo)));
}

void CallWatcher::setTurnServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName = "turn." + m_client->configuration().domain();
    m_turnPort = 3478;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        m_turnPort = serviceInfo.records().first().port();
    }

    // lookup TURN host name
    QHostInfo::lookupHost(serverName, this, SLOT(setTurnServer(QHostInfo)));
}

void CallWatcher::setTurnServer(const QHostInfo &hostInfo)
{
    if (hostInfo.addresses().isEmpty()) {
        warning(QString("Could not lookup TURN server %1").arg(hostInfo.hostName()));
        return;
    }
    m_callManager->setTurnServer(hostInfo.addresses().first(), m_turnPort);
    m_callManager->setTurnUser(m_client->configuration().user());
    m_callManager->setTurnPassword(m_client->configuration().password());
}

// PLUGIN

class CallsPlugin : public ChatPlugin
{
public:
    void finalize(Chat *chat);
    bool initialize(Chat *chat);
    QString name() const { return "Calls"; };
    void polish(Chat *chat, ChatPanel *panel);

private:
    QMap<Chat*, CallWatcher*> m_watchers;
};

bool CallsPlugin::initialize(Chat *chat)
{
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");

    qmlRegisterUncreatableType<QXmppCall>("QXmpp", 0, 4, "QXmppCall", "");
    qmlRegisterUncreatableType<CallAudioHelper>("wiLink", 1, 2, "CallAudioHelper", "");
    qmlRegisterType<CallVideoHelper>("wiLink", 1, 2, "CallVideoHelper");
    qmlRegisterType<CallVideoItem>("wiLink", 1, 2, "CallVideoItem");
    qmlRegisterUncreatableType<DeclarativePen>("wiLink", 1, 2, "DeclarativePen", "");

    CallWatcher *watcher = new CallWatcher(chat);
    m_watchers.insert(chat, watcher);
    return true;
}

void CallsPlugin::finalize(Chat *chat)
{
    m_watchers.remove(chat);
}

void CallsPlugin::polish(Chat *chat, ChatPanel *panel)
{
    CallWatcher *watcher = m_watchers.value(chat);
    if (!watcher || !qobject_cast<ChatDialogPanel*>(panel))
        return;

    const QStringList fullJids = chat->rosterModel()->contactFeaturing(panel->objectName(), ChatRosterModel::VoiceFeature);
    if (!fullJids.isEmpty()) {
        QAction *action = panel->addAction(QIcon(":/call.png"), QObject::tr("Call"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), watcher, SLOT(callContact()));
    }
}

Q_EXPORT_STATIC_PLUGIN2(calls, CallsPlugin)

