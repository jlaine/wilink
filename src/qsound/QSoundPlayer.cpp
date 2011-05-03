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

#include <QAudioOutput>
#include <QVariant>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

QSoundPlayer::QSoundPlayer(QObject *parent)
    : QObject(parent),
    m_readerId(0)
{
    m_audioDevice = QAudioDeviceInfo::defaultOutputDevice();
}

int QSoundPlayer::play(const QString &name, bool repeat)
{
    if (name.isEmpty())
        return 0;
    QSoundFile *reader = new QSoundFile(name, this);
    reader->setRepeat(repeat);
    return play(reader);
}

int QSoundPlayer::play(QSoundFile *reader)
{
    if (!reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        delete reader;
        return 0;
    }

    // register reader
    const int id = ++m_readerId;
    m_readers[id] = reader;

    // move reader to audio thread
    reader->setParent(0);
    reader->moveToThread(thread());
    reader->setParent(this);

    // schedule play
    QMetaObject::invokeMethod(this, "_q_start", Q_ARG(int, id));

    return id;
}

void QSoundPlayer::setAudioOutputDevice(const QAudioDeviceInfo &device)
{
    m_audioDevice = device;
}

void QSoundPlayer::stop(int id)
{
    // schedule stop
    QMetaObject::invokeMethod(this, "_q_stop", Q_ARG(int, id));
}

void QSoundPlayer::_q_start(int id)
{
    QSoundFile *reader = m_readers.value(id);
    if (!reader)
        return;

    const QAudioFormat format = reader->format();
    QAudioOutput *output = new QAudioOutput(m_audioDevice, format, reader);
    output->setProperty("_play_id", m_readerId);
    connect(output, SIGNAL(stateChanged(QAudio::State)), this, SLOT(_q_stateChanged(QAudio::State)));
    output->start(reader);
}

void QSoundPlayer::_q_stop(int id)
{
    QSoundFile *reader = m_readers.value(id);
    if (!reader)
        return;

    reader->close();
}

void QSoundPlayer::_q_stateChanged(QAudio::State state)
{
    QAudioOutput *output = qobject_cast<QAudioOutput*>(sender());
    if (!output || state == QAudio::ActiveState)
        return;

    int id = output->property("_play_id").toInt();
    QSoundFile *reader = m_readers.take(id);
    if (reader) {
        reader->deleteLater();
        emit finished(id);
    }
}

