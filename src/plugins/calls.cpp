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
#include <QMenu>
#include <QMessageBox>
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

Generator::Generator(const QAudioFormat &format,
                     qint64 durationUs,
                     int frequency,
                     QObject *parent)
    : QObject(parent)
{
    generateData(format, durationUs, frequency);
    m_timer = new QTimer(this);
    m_timer->setInterval(durationUs/1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

Generator::~Generator()
{

}

void Generator::start(QIODevice *device)
{
    m_device = device;
    m_timer->start();
}

void Generator::stop()
{
    m_timer->stop();
    m_device = 0;
}

void Generator::generateData(const QAudioFormat &format, qint64 durationUs, int frequency)
{
    const int channelBytes = format.sampleSize() / 8;
    const int sampleBytes = format.channels() * channelBytes;

    qint64 length = (format.frequency() * format.channels() * (format.sampleSize() / 8))
                    * durationUs / 100000;

    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes) // suppress warning in release builds

    m_buffer.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());
    int sampleIndex = 0;
    //qDebug() << "size" << length;

    while (length) {
        const qreal x = qSin(2 * M_PI * frequency * qreal(sampleIndex % format.frequency()) / format.frequency());
        for (int i=0; i<format.channels(); ++i) {
            if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::UnSignedInt) {
                const quint8 value = static_cast<quint8>((1.0 + x) / 2 * 255);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::SignedInt) {
                const qint8 value = static_cast<qint8>(x * 127);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::UnSignedInt) {
                quint16 value = static_cast<quint16>((1.0 + x) / 2 * 65535);
                if (format.byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<quint16>(value, ptr);
                else
                    qToBigEndian<quint16>(value, ptr);
            } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::SignedInt) {
                qint16 value = static_cast<qint16>(x * 32767);
                if (format.byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<qint16>(value, ptr);
                else
                    qToBigEndian<qint16>(value, ptr);
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
        ++sampleIndex;
    }
}

void Generator::tick()
{
    if (m_device)
        m_device->write(m_buffer);
}

CallPanel::CallPanel(QXmppCall *call, QWidget *parent)
    : ChatPanel(parent), m_call(call)
{
    connect(call, SIGNAL(buffered()), this, SLOT(callBuffered()));
    connect(call, SIGNAL(connected()), this, SLOT(callConnected()));

    QTimer::singleShot(0, this, SIGNAL(showPanel()));
}

void CallPanel::callBuffered()
{
    QAudioFormat format = formatFor(m_call->payloadType());
    QAudioOutput *audioOutput = new QAudioOutput(format, this);
    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));

    if (m_call->direction() == QXmppCall::IncomingDirection)
    {
        qDebug() << "start playback";
        audioOutput->start(m_call);
    }
}

void CallPanel::callConnected()
{
    QAudioFormat format = formatFor(m_call->payloadType());

    //QAudioInput *audioInput = new QAudioInput(m_format, this);
    if (m_call->direction() == QXmppCall::OutgoingDirection)
    {
        qDebug() << "start capture";
        Generator *generator = new Generator(format, 100000, ToneFrequencyHz, this);
        generator->start(m_call);
        //audioInput->start(m_call);
    }
}

void CallPanel::stateChanged(QAudio::State state)
{
    qDebug() << "state changed" << state;
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
        CallPanel *panel = new CallPanel(call, m_window);
        m_window->addPanel(panel);
        call->accept();
    } else
        call->hangup();
}

void CallWatcher::callContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QXmppCall *call = m_client->callManager().call(fullJid);
    CallPanel *panel = new CallPanel(call, m_window);
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
        QStringList fullJids = m_roster->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
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

