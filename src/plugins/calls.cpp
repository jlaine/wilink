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

#include "calls.h"

#include "application.h"
#include "chat.h"
#include "chat_panel.h"
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

CallVideoWidget::CallVideoWidget(QGraphicsItem *parent)
    : QGraphicsItem(parent),
    m_boundingRect(0, 0, 0, 0)
{
}

QRectF CallVideoWidget::boundingRect() const
{
    return m_boundingRect;
}

void CallVideoWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->drawImage(m_boundingRect, m_image);
}

void CallVideoWidget::present(const QXmppVideoFrame &frame)
{
    if (!frame.isValid() || frame.size() != m_image.size())
        return;
    QVideoGrabber::frameToImage(&frame, &m_image);
    update(m_boundingRect);
}

void CallVideoWidget::setFormat(const QXmppVideoFormat &format)
{
    const QSize size = format.frameSize();
    if (size != m_image.size()) {
        m_image = QImage(size, QImage::Format_RGB32);
        m_image.fill(0);
    }
}

void CallVideoWidget::setSize(const QSizeF &size)
{
    m_boundingRect = QRectF(QPointF(0, 0), size);
}

/** Sets the video capture format.
 */
void CallWidget::setCaptureFormat(const QXmppVideoFormat &format)
{
    m_videoMonitor->setFormat(format);
    m_videoMonitor->setSize(QSizeF(format.frameSize().width() / 2, format.frameSize().height() / 2));
    updateGeometry();
}

/** Sets the video playback format.
 */
void CallWidget::setPlaybackFormat(const QXmppVideoFormat &format)
{
    m_videoOutput->setFormat(format);
    m_videoOutput->setSize(format.frameSize());
    m_videoMonitor->setPos(format.frameSize().width(), 0);
    m_label->setPos(0, format.frameSize().height());
    updateGeometry();
}

/** Sets the status text.
 */
void CallWidget::setStatus(const QString &status)
{
    m_label->setPlainText(status);
}

QSizeF CallWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        const QSizeF outputSize = m_videoOutput->boundingRect().size();
        const QSizeF monitorSize = m_videoMonitor->boundingRect().size();
        QSizeF hint = m_label->effectiveSizeHint(which);
        hint.setWidth(qMax(hint.width(), outputSize.width() + monitorSize.width()));
        hint.setHeight(hint.height() + qMax(outputSize.height(), monitorSize.height()));
        return hint;
    } else {
        return constraint;
    }
}

CallWidget::CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_audioInput(0),
    m_audioOutput(0),
    m_videoButton(0),
    m_videoConversion(0),
    m_videoGrabber(0),
    m_call(call),
    m_soundId(0)
{
    bool check;

    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");

    // video timer
    m_videoTimer = new QTimer(this);

    // setup GUI
    m_label = new ChatPanelText(tr("Connecting.."), this);
    m_videoOutput = new CallVideoWidget(this);
    m_videoMonitor = new CallVideoWidget(this);

    const QStringList videoJids = rosterModel->contactFeaturing(m_call->jid(), ChatRosterModel::VideoFeature);
    if (videoJids.contains(m_call->jid())) {
        m_videoButton = new ChatPanelButton(this);
        m_videoButton->setPixmap(QPixmap(":/camera.png"));
        //addButton(videoButton);
        connect(m_videoButton, SIGNAL(clicked()), m_call, SLOT(startVideo()));
    }

    m_hangupButton = new ChatPanelButton(this);
    m_hangupButton->setPixmap(QPixmap(":/hangup.png"));
    m_hangupButton->setToolTip(tr("Hang up"));
    //addButton(m_button);

    // connect signals
    check = connect(this, SIGNAL(destroyed(QObject*)),
                    m_call, SLOT(hangup()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(destroyed(QObject*)),
                    m_call, SLOT(deleteLater()));
    Q_ASSERT(check);

    check = connect(m_call, SIGNAL(ringing()),
                    this, SLOT(callRinging()));
    Q_ASSERT(check);

    check = connect(m_call, SIGNAL(stateChanged(QXmppCall::State)),
                    this, SLOT(callStateChanged(QXmppCall::State)));
    Q_ASSERT(check);

    check = connect(m_call, SIGNAL(audioModeChanged(QIODevice::OpenMode)),
                    this, SLOT(audioModeChanged(QIODevice::OpenMode)));
    Q_ASSERT(check);

    check = connect(m_call, SIGNAL(videoModeChanged(QIODevice::OpenMode)),
                    this, SLOT(videoModeChanged(QIODevice::OpenMode)));
    Q_ASSERT(check);
    
    check = connect(m_hangupButton, SIGNAL(clicked()),
                    m_call, SLOT(hangup()));
    Q_ASSERT(check);

    check = connect(m_videoTimer, SIGNAL(timeout()),
                    this, SLOT(videoRefresh()));
    Q_ASSERT(check);
}

CallWidget::~CallWidget()
{
    // stop tone
    if (m_soundId)
        wApp->soundPlayer()->stop(m_soundId);
}

void CallWidget::audioModeChanged(QIODevice::OpenMode mode)
{
    qDebug("audio mode changed %i", (int)mode);
    QXmppRtpAudioChannel *channel = m_call->audioChannel();
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
        m_audioOutput->setBufferSize(bufferSize);
        connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
        m_audioOutput->start(channel);
    } else if (!canRead && m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = 0;
    }

    // start or stop capture
    const bool canWrite = (mode & QIODevice::WriteOnly);
    if (canWrite && !m_audioInput) {
        m_audioInput = new QAudioInput(wApp->audioInputDevice(), format, this);
        m_audioInput->setBufferSize(bufferSize);
        connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
        m_audioInput->start(channel);
    } else if (!canWrite && m_audioOutput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = 0;
    }
}

void CallWidget::audioStateChanged(QAudio::State state)
{
    QObject *audio = sender();
    if (!audio)
        return;
    if (audio == m_audioInput)
    {
        warning(QString("Audio input state %1 error %2").arg(
            QString::number(m_audioInput->state()),
            QString::number(m_audioInput->error())));

        // restart audio input if we get an underrun
        if (m_audioInput->state() == QAudio::StoppedState &&
            m_audioInput->error() == QAudio::UnderrunError)
        {
            warning("Audio input needs restart due to buffer underrun");
            m_audioInput->start(m_call->audioChannel());
        }
    } else if (audio == m_audioOutput) {
        debug(QString("Audio output state %1 error %2").arg(
            QString::number(m_audioOutput->state()),
            QString::number(m_audioOutput->error())));

        // restart audio output if we get an underrun
        if (m_audioOutput->state() == QAudio::StoppedState &&
            m_audioOutput->error() == QAudio::UnderrunError)
        {
            warning("Audio output needs restart due to buffer underrun");
            m_audioOutput->start(m_call->audioChannel());
        }
    }
}

void CallWidget::callRinging()
{
    // we only want ring events in the connecting state
    if (m_call->state() != QXmppCall::ConnectingState)
        return;

    setStatus(tr("Ringing.."));

    // start outgoing tone
    if (!m_soundId)
        m_soundId = wApp->soundPlayer()->play(":/call-outgoing.ogg", true);
}

void CallWidget::callStateChanged(QXmppCall::State state)
{
    // stop tone
    if (m_soundId && state != QXmppCall::ConnectingState) {
        wApp->soundPlayer()->stop(m_soundId);
        m_soundId = 0;
    }

    // update status
    switch (state)
    {
    case QXmppCall::ConnectingState:
        setStatus(tr("Connecting.."));
        break;
    case QXmppCall::ActiveState:
        setStatus(tr("Call connected."));
        break;
    case QXmppCall::DisconnectingState:
        setStatus(tr("Disconnecting.."));
        m_hangupButton->setEnabled(false);
        break;
    case QXmppCall::FinishedState:
        setStatus(tr("Call finished."));
        m_hangupButton->setEnabled(false);
        break;
    }

    // make widget disappear
    if (state == QXmppCall::FinishedState)
        QTimer::singleShot(1000, this, SIGNAL(finished()));
}

void CallWidget::videoModeChanged(QIODevice::OpenMode mode)
{
    qDebug("video mode changed %i", (int)mode);
    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        mode = QIODevice::NotOpen;

    // start or stop playback
    const bool canRead = (mode & QIODevice::ReadOnly);
    if (canRead && !m_videoTimer->isActive()) {
        QXmppVideoFormat format = channel->decoderFormat();
        setPlaybackFormat(format);
        m_videoTimer->start(1000 / format.frameRate());
    } else if (!canRead && m_videoTimer->isActive()) {
        m_videoTimer->stop();
        updateGeometry();
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
        setCaptureFormat(m_videoGrabber->format());
    } else if (!canWrite && m_videoGrabber) {
        m_videoGrabber->stop();
        delete m_videoGrabber;
        m_videoGrabber = 0;
        if (m_videoConversion) {
            delete m_videoConversion;
            m_videoConversion = 0;
        }
    }

    // update geometry
    updateGeometry();
}

void CallWidget::videoCapture(const QXmppVideoFrame &frame)
{
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
        m_videoMonitor->present(frame);
    }
}

void CallWidget::videoRefresh()
{
    QXmppRtpVideoChannel *channel = m_call->videoChannel();
    if (!channel)
        return;

    QList<QXmppVideoFrame> frames = channel->readFrames();
    if (!frames.isEmpty())
        m_videoOutput->present(frames.last());
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
    CallWidget *widget = new CallWidget(call, m_window->rosterModel());
    const QString bareJid = jidToBareJid(call->jid());
    QModelIndex index = m_window->rosterModel()->findItem(bareJid);
    if (index.isValid())
        QMetaObject::invokeMethod(m_window, "rosterClicked", Q_ARG(QModelIndex, index));

    ChatPanel *panel = m_window->panel(bareJid);
    if (panel)
        panel->addWidget(widget);
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

void CallWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    const QStringList fullJids = m_window->rosterModel()->contactFeaturing(jid, ChatRosterModel::VoiceFeature);
    if (!m_client->isConnected() || fullJids.isEmpty())
        return;

    QAction *action = menu->addAction(QIcon(":/call.png"), tr("Call"));
    action->setData(fullJids.first());
    connect(action, SIGNAL(triggered()), this, SLOT(callContact()));
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
    bool initialize(Chat *chat);
    QString name() const { return "Calls"; };
};

bool CallsPlugin::initialize(Chat *chat)
{
    CallWatcher *watcher = new CallWatcher(chat);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            watcher, SLOT(rosterMenu(QMenu*, QModelIndex)));

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(calls, CallsPlugin)

