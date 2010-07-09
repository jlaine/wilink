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

const int DurationSeconds = 1;
const int ToneFrequencyHz = 600;
const int DataFrequencyHz = 11025;
//const int DataFrequencyHz = 44100;
const int BufferSize      = 32768;

Generator::Generator(const QAudioFormat &format,
                     qint64 durationUs,
                     int frequency,
                     QObject *parent)
                         :   QIODevice(parent)
                         ,   m_pos(0)
{
    generateData(format, durationUs, frequency);
    m_timer = new QTimer(this);
    m_timer->setInterval(durationUs/1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

Generator::~Generator()
{

}

void Generator::start()
{
    open(QIODevice::ReadOnly);
    m_timer->start();
}

void Generator::stop()
{
    m_pos = 0;
    close();
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

qint64 Generator::bufferSize() const
{
    return m_buffer.size();
}

qint64 Generator::readData(char *data, qint64 len)
{
    qDebug() << "read req" << len;
    const qint64 chunk = qMin((m_buffer.size() - m_pos), len);
    memcpy(data, m_buffer.constData() + m_pos, chunk);
    //m_pos = (m_pos + chunk) % m_buffer.size();
    m_pos += chunk;
    return chunk;
}

void Generator::tick()
{
    m_pos = 0;
    emit readyRead();
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_buffer.size() - m_pos;
}

CallWatcher::CallWatcher(Chat *chatWindow)
    : QObject(chatWindow), m_window(chatWindow), m_call(0)
{
    m_client = chatWindow->client();
    m_roster = chatWindow->rosterModel();

    connect(&m_client->callManager(), SIGNAL(callReceived(QXmppCall*)),
            this, SLOT(callReceived(QXmppCall*)));

    QAudioFormat m_format;
    m_format.setFrequency(DataFrequencyHz);
    m_format.setChannels(1);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    m_audioInput = new QAudioInput(m_format, this);
    m_audioOutput = new QAudioOutput(m_format, this);
    connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));
    m_generator = new Generator(m_format, 100000, ToneFrequencyHz, this);
    connect(m_generator, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

void CallWatcher::callClicked(QAbstractButton * button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    QXmppCall *call = qobject_cast<QXmppCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    if (box->standardButton(button) == QMessageBox::Yes)
    {
        call->accept();
    } else
        call->hangup();
}

void CallWatcher::callConnected()
{
    QXmppCall *call = qobject_cast<QXmppCall*>(sender());
    if (call->direction() == QXmppCall::IncomingDirection)
    {
        qDebug() << "start playback";
        //m_generator->start();
        //m_audioOutput->start(m_generator);
        m_audioOutput->start(call);
    }
    else {
        qDebug() << "start capture";
        m_call = call;
        m_generator->start();
        //m_audioInput->start(call);
    }
}

void CallWatcher::callContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QXmppCall *call = m_client->callManager().call(fullJid);
    connect(call, SIGNAL(connected()), this, SLOT(callConnected()));
}

void CallWatcher::callReceived(QXmppCall *call)
{
    // FIXME : for debugging purposes, always accept calls
    connect(call, SIGNAL(connected()), this, SLOT(callConnected()));
    call->accept();
    return;

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

void CallWatcher::readyRead()
{
    if (!m_call)
        return;

    QByteArray chunk = m_generator->read(m_generator->bufferSize());
    qDebug() << "read chunk" << chunk.size();
    m_call->write(chunk);
}

void CallWatcher::stateChanged(QAudio::State state)
{
    qDebug() << "state changed" << state;
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

