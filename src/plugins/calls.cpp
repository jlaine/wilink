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

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QtCore/qmath.h>
#include <QtCore/qendian.h>
#include <QThread>
#include <QTimer>

#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppUtils.h"

#include "calls.h"

#include "chat.h"
#include "chat_panel.h"
#include "chat_plugin.h"
#include "chat_roster.h"

const int ToneFrequencyHz = 600;

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

Reader::Reader(const QAudioFormat &format, QObject *parent)
    : QObject(parent)
{
    int durationMs = 20;

    // 100ms
    m_block = (format.frequency() * format.channels() * (format.sampleSize() / 8)) * durationMs / 1000;
    m_input = new QFile(QString("test-%1.raw").arg(format.frequency()), this);
    if (!m_input->open(QIODevice::ReadOnly))
        qWarning() << "Could not open" << m_input->fileName();
    m_timer = new QTimer(this);
    m_timer->setInterval(durationMs);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void Reader::start(QIODevice *device)
{
    m_output = device;
    m_timer->start();
}

void Reader::stop()
{
    m_timer->stop();
}

void Reader::tick()
{
    QByteArray block(m_block, 0);
    if (m_input->read(block.data(), block.size()) > 0)
        m_output->write(block);
    else
        qWarning() << "Reader could not read from" << m_input->fileName();
}

CallHandler::CallHandler(QXmppCall *call, QThread *mainThread)
    : m_call(call),
    m_audioInput(0),
    m_audioOutput(0),
    m_mainThread(mainThread)
{
    m_call->setParent(this);
    connect(m_call, SIGNAL(stateChanged(QXmppCall::State)),
        this, SLOT(callStateChanged(QXmppCall::State)));
}

void CallHandler::audioStateChanged(QAudio::State state)
{
    QObject *audio = sender();
    if (!audio)
        return;
    if (audio == m_audioInput)
    {
#ifndef FAKE_AUDIO_INPUT
        if (m_audioInput->error() != QAudio::NoError)
            qWarning() << "audio input state" << state << "error" << m_audioInput->error();
        if (m_audioInput->error() == QAudio::UnderrunError)
            m_audioInput->start(m_call);
#endif
    } else if (audio == m_audioOutput) {
        if (m_audioOutput->error() != QAudio::NoError)
            qWarning() << "audio output state" << state << "error" << m_audioOutput->error();
    }
}

void CallHandler::callStateChanged(QXmppCall::State state)
{
    // start or stop capture
    if (state == QXmppCall::ActiveState)
    {
        QAudioFormat format = formatFor(m_call->payloadType());
        // the size in bytes of the audio samples for a single RTP packet
        int packetSize = (format.frequency() * format.channels() * (format.sampleSize() / 8)) * m_call->payloadType().ptime() / 1000;

        if (!m_audioOutput)
        {
            m_audioOutput = new QAudioOutput(format, this);
            m_audioOutput->setBufferSize(2 * packetSize);
            connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
            m_audioOutput->start(m_call);
        }

        if (!m_audioInput)
        {
#ifdef FAKE_AUDIO_INPUT
            m_audioInput = new Reader(format, this);
#else
            m_audioInput = new QAudioInput(format, this);
            m_audioInput->setBufferSize(2 * packetSize);
            connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
#endif
            m_audioInput->start(m_call);
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

    // handle completion
    if (state == QXmppCall::FinishedState)
    {
        moveToThread(m_mainThread);
        emit finished();
    }
}

CallWidget::CallWidget(QXmppCall *call, ChatRosterModel *rosterModel, QGraphicsItem *parent)
    : ChatPanelWidget(parent),
    m_call(call)
{
    // CALL THREAD
    m_callThread = new QThread;
    m_callHandler = new CallHandler(m_call, QThread::currentThread());
    connect(m_callHandler, SIGNAL(finished()), m_callThread, SLOT(quit()));
    connect(m_callThread, SIGNAL(finished()), this, SLOT(callFinished()));
    m_callHandler->moveToThread(m_callThread);
    m_callThread->start();

    setIconPixmap(QPixmap(":/call.png"));
    setButtonPixmap(QPixmap(":/hangup.png"));
    setButtonToolTip(tr("Hang up"));

    m_statusLabel = new QGraphicsSimpleTextItem(tr("Connecting.."), this);
    m_statusLabel->setPos(32, 0);

    m_hangupButton = new QPushButton;
    m_hangupButton->setFlat(true);
    m_hangupButton->setIcon(QIcon(":/hangup.png"));
    m_hangupButton->setMaximumWidth(32);
    m_hangupButton->setToolTip(tr("Hang up"));
    connect(m_hangupButton, SIGNAL(clicked()), m_call, SLOT(hangup()));

    //box->addWidget(m_hangupButton);

    connect(m_call, SIGNAL(ringing()), this, SLOT(ringing()));
    connect(m_call, SIGNAL(stateChanged(QXmppCall::State)),
        this, SLOT(callStateChanged(QXmppCall::State)));
}

/** When the call thread finishes, perform cleanup.
 */
void CallWidget::callFinished()
{
    m_callHandler->deleteLater();
    m_callThread->wait();
    delete m_callThread;

    // make widget disappear
    m_hangupButton->setEnabled(false);
    QTimer::singleShot(1000, this, SLOT(disappear()));
}

void CallWidget::callStateChanged(QXmppCall::State state)
{
    // update status
    switch (state)
    {
    case QXmppCall::OfferState:
    case QXmppCall::ConnectingState:
        m_statusLabel->setText(tr("Connecting.."));
        break;
    case QXmppCall::ActiveState:
        m_statusLabel->setText(tr("Call connected."));
        break;
    case QXmppCall::DisconnectingState:
        m_statusLabel->setText(tr("Disconnecting.."));
        m_hangupButton->setEnabled(false);
        break;
    case QXmppCall::FinishedState:
        m_statusLabel->setText(tr("Call finished."));
        m_hangupButton->setEnabled(false);
        break;
    }
}

void CallWidget::ringing()
{
    m_statusLabel->setText(tr("Ringing.."));
}

CallWatcher::CallWatcher(Chat *chatWindow)
    : QObject(chatWindow), m_window(chatWindow)
{
    m_client = chatWindow->client();

    connect(&m_client->callManager(), SIGNAL(callReceived(QXmppCall*)),
            this, SLOT(callReceived(QXmppCall*)));
}

void CallWidget::setGeometry(const QRectF &rect)
{
    m_statusLabel->setPos(32, (rect.height() - m_statusLabel->boundingRect().height()) / 2);
    ChatPanelWidget::setGeometry(rect);
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

void CallWatcher::callClicked(QAbstractButton * button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    QXmppCall *call = qobject_cast<QXmppCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    if (box->standardButton(button) == QMessageBox::Yes)
    {
        disconnect(call, SIGNAL(finished()), call, SLOT(deleteLater()));
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

    QXmppCall *call = m_client->callManager().call(fullJid);
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

    /* connect signals */
    connect(call, SIGNAL(finished()), call, SLOT(deleteLater()));
    connect(call, SIGNAL(finished()), box, SLOT(deleteLater()));
    connect(box, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(callClicked(QAbstractButton*)));
    box->show();
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

// PLUGIN

class CallsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
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

