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
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QHostInfo>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "QSoundPlayer.h"
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

CallWidget::CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent)
    : ChatPanelWidget(parent),
    m_audioInput(0),
    m_audioOutput(0),
    m_call(call),
    m_soundId(0)
{
    //m_call->setParent(this);

    // setup GUI
    const QStringList videoJids = rosterModel->contactFeaturing(m_call->jid(), ChatRosterModel::VideoFeature);
    if (videoJids.contains(m_call->jid())) {
        ChatPanelButton *videoButton = new ChatPanelButton(this);
        videoButton->setPixmap(QPixmap(":/photos.png"));
        addButton(videoButton);
        connect(videoButton, SIGNAL(clicked()), m_call, SLOT(startVideo()));
    }

    m_button = new ChatPanelButton(this);
    m_button->setPixmap(QPixmap(":/hangup.png"));
    m_button->setToolTip(tr("Hang up"));
    addButton(m_button);
    connect(m_button, SIGNAL(clicked()), m_call, SLOT(hangup()));

    setIconPixmap(QPixmap(":/call.png"));
    m_label = new QGraphicsSimpleTextItem(tr("Connecting.."), this);
    connect(m_call, SIGNAL(ringing()), this, SLOT(callRinging()));
    connect(m_call, SIGNAL(stateChanged(QXmppCall::State)),
        this, SLOT(callStateChanged(QXmppCall::State)));
}

CallWidget::~CallWidget()
{
    // stop tone
    if (m_soundId)
        wApp->soundPlayer()->stop(m_soundId);
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
    // we only want ring events in the "offer" state
    if (m_call->state() != QXmppCall::OfferState)
        return;

    m_label->setText(tr("Ringing.."));

    // start outgoing tone
    if (!m_soundId)
        m_soundId = wApp->soundPlayer()->play(":/call-outgoing.ogg", true);
}

void CallWidget::callStateChanged(QXmppCall::State state)
{
    // stop tone
    if (m_soundId && state != QXmppCall::OfferState) {
        wApp->soundPlayer()->stop(m_soundId);
        m_soundId = 0;
    }

    // start or stop capture
    if (state == QXmppCall::ActiveState)
    {
        QXmppRtpChannel *channel = m_call->audioChannel();
        QAudioFormat format = formatFor(channel->payloadType());

#ifdef Q_OS_MAC
        // 128ms at 8kHz
        const int bufferSize = 2048 * format.channels();
#else
        // 160ms at 8kHz
        const int bufferSize = 2560 * format.channels();
#endif

        if (!m_audioOutput)
        {
            m_audioOutput = new QAudioOutput(wApp->audioOutputDevice(), format, this);
            m_audioOutput->setBufferSize(bufferSize);
            connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
            m_audioOutput->start(channel);
        }

        if (!m_audioInput)
        {
            m_audioInput = new QAudioInput(wApp->audioInputDevice(), format, this);
            m_audioInput->setBufferSize(bufferSize);
            connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
            m_audioInput->start(channel);
        }
    } else {
        if (m_audioInput)
        {
            m_audioInput->stop();
            delete m_audioInput;
            m_audioInput = 0;
        }
        if (m_audioOutput)
        {
            m_audioOutput->stop();
            delete m_audioOutput;
            m_audioOutput = 0;
        }
    }

    // update status
    switch (state)
    {
    case QXmppCall::OfferState:
    case QXmppCall::ConnectingState:
        m_label->setText(tr("Connecting.."));
        break;
    case QXmppCall::ActiveState:
        m_label->setText(tr("Call connected."));
        break;
    case QXmppCall::DisconnectingState:
        m_label->setText(tr("Disconnecting.."));
        m_button->setEnabled(false);
        break;
    case QXmppCall::FinishedState:
        m_label->setText(tr("Call finished."));
        m_button->setEnabled(false);
        break;
    }

    // make widget disappear
    if (state == QXmppCall::FinishedState)
        QTimer::singleShot(1000, this, SLOT(disappear()));
}

void CallWidget::setGeometry(const QRectF &rect)
{
    ChatPanelWidget::setGeometry(rect);

    QRectF contents = contentsRect();
    m_label->setPos(contents.left(), contents.top() +
        (contents.height() - m_label->boundingRect().height()) / 2);
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

