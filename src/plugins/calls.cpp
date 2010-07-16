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
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QtCore/qmath.h>
#include <QtCore/qendian.h>
#include <QTimer>

#include "QXmppCallManager.h"
#include "QXmppUtils.h"

#include "calls.h"

#include "chat.h"
#include "chat_client.h"
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
        qDebug() << "Could not open" << m_input->fileName();
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

CallPanel::CallPanel(QXmppCall *call, ChatRosterModel *rosterModel, QWidget *parent)
    : ChatPanel(parent),
    m_call(call),
    m_audioInput(0),
    m_audioOutput(0)
{
    const QString bareJid = jidToBareJid(call->jid());
    const QString contactName = rosterModel->contactName(bareJid);

    setObjectName(QString("call/%1").arg(m_call->sid()));
    setWindowIcon(QIcon(":/chat.png"));
    setWindowTitle(tr("Call with %1").arg(contactName));
    setWindowExtra(QString("<br/>%1").arg(bareJid));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    // HEADER

    layout->addItem(headerLayout());

    // STATUS

    layout->addStretch();
    QHBoxLayout *box = new QHBoxLayout;

    m_imageLabel = new QLabel;
    m_imageLabel->setPixmap(QPixmap(":/peer-128.png"));
    box->addWidget(m_imageLabel);

    m_statusLabel = new QLabel;
    m_statusLabel->setText(tr("Connecting.."));
    box->addWidget(m_statusLabel);

    layout->addLayout(box);
    layout->addStretch();

    // BUTTONS

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addStretch();
    m_hangupButton = new QPushButton(tr("Hang up"));
    connect(m_hangupButton, SIGNAL(clicked()), m_call, SLOT(hangup()));
    hbox->addWidget(m_hangupButton);

    layout->addItem(hbox);

    setLayout(layout);

    connect(this, SIGNAL(hidePanel()), call, SLOT(hangup()));
    connect(this, SIGNAL(hidePanel()), this, SIGNAL(unregisterPanel()));
    connect(call, SIGNAL(finished()), this, SLOT(finished()));
    connect(call, SIGNAL(ringing()), this, SLOT(ringing()));
    connect(call, SIGNAL(openModeChanged(QIODevice::OpenMode)), this, SLOT(openModeChanged(QIODevice::OpenMode)));

    QTimer::singleShot(0, this, SIGNAL(showPanel()));
}

void CallPanel::audioStateChanged(QAudio::State state)
{
    QObject *audio = sender();
    if (!audio)
        return;
    if (audio == m_audioInput)
    {
        qDebug() << "audio input state changed" << state;
        if (m_audioInput->error() != QAudio::NoError)
            qWarning() << "audio input error" << m_audioInput->error();
    } else if (audio == m_audioOutput) {
        qDebug() << "audio output state changed" << state;
        if (m_audioOutput->error() != QAudio::NoError)
            qWarning() << "audio output error" << m_audioOutput->error();
    }
}

void CallPanel::finished()
{
    m_statusLabel->setText(tr("Call finished."));
    m_hangupButton->setEnabled(false);
}

void CallPanel::openModeChanged(QIODevice::OpenMode mode)
{
    QAudioFormat format = formatFor(m_call->payloadType());

    // capture
    if ((mode & QIODevice::WriteOnly) && !m_audioInput)
    {
        qDebug() << "start capture";
#ifdef FAKE_AUDIO_INPUT
        m_audioInput = new Reader(format, this);
#else
        m_audioInput = new QAudioInput(format, this);
        connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
#endif
        m_audioInput->start(m_call);
    }
    else if (!(mode & QIODevice::WriteOnly) && m_audioInput)
    {
        qDebug() << "stop capture";
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = 0;
    }

    // playback
    if ((mode & QIODevice::ReadOnly) && !m_audioOutput)
    {
        qDebug() << "start playback";
        m_audioOutput = new QAudioOutput(format, this);
        connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
        m_audioOutput->start(m_call);
    }
    else if (!(mode & QIODevice::ReadOnly) && m_audioOutput)
    {
        qDebug() << "stop playback";
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = 0;
    }

    // status
    if (mode == QIODevice::ReadWrite)
        m_statusLabel->setText(tr("Call connected."));
}

void CallPanel::ringing()
{
    m_statusLabel->setText(tr("Ringing.."));
}

CallWatcher::CallWatcher(Chat *chatWindow)
    : QObject(chatWindow), m_window(chatWindow)
{
    m_client = chatWindow->client();
    m_roster = chatWindow->rosterModel();

    connect(&m_client->callManager(), SIGNAL(callReceived(QXmppCall*)),
            this, SLOT(callReceived(QXmppCall*)));
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
        CallPanel *panel = new CallPanel(call, m_roster, m_window);
        m_window->addPanel(panel);
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
    CallPanel *panel = new CallPanel(call, m_roster, m_window);
    m_window->addPanel(panel);
}

void CallWatcher::callReceived(QXmppCall *call)
{
    const QString bareJid = jidToBareJid(call->jid());
    const QString contactName = m_roster->contactName(bareJid);

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
    if (!m_client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Contact)
    {
        QStringList fullJids = m_roster->contactFeaturing(jid, ChatRosterModel::VoiceFeature);
        if (fullJids.isEmpty())
            return;

        QAction *action = menu->addAction(tr("Call"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), this, SLOT(callContact()));
    }
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

