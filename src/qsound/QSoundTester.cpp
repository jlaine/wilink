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

#include <QAudioInput>
#include <QAudioOutput>
#include <QBuffer>
#include <QTimer>

#include "QSoundMeter.h"
#include "QSoundStream.h"
#include "QSoundTester.h"

QSoundTester::QSoundTester(QObject *parent)
    : QSoundPlayer(parent),
    m_state(IdleState),
    m_volume(0)
{
    m_buffer = new QBuffer(this);

    // prepare audio format
    m_format.setFrequency(8000);
    m_format.setChannels(1);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    m_stream = new QSoundStream(this);
    connect(m_stream, SIGNAL(inputVolumeChanged(int)),
            this, SLOT(_q_volumeChanged(int)));
    connect(m_stream, SIGNAL(outputVolumeChanged(int)),
            this, SLOT(_q_volumeChanged(int)));
}

int QSoundTester::duration() const
{
    return 5;
}

int QSoundTester::maximumVolume() const
{
    return QSoundMeter::maximum();
}

void QSoundTester::start(const QString &inputDeviceName, const QString &outputDeviceName)
{
    setInputDeviceName(inputDeviceName);
    setOutputDeviceName(outputDeviceName);

    // start input
    m_buffer->open(QIODevice::WriteOnly);
    m_buffer->reset();
    m_stream->startInput(m_format, m_buffer);
    QTimer::singleShot(duration() * 1000, this, SLOT(_q_playback()));

    // update state
    m_state = RecordingState;
    emit stateChanged(m_state);
}

QSoundTester::State QSoundTester::state() const
{
    return m_state;
}

int QSoundTester::volume() const
{
    return m_volume;
}

void QSoundTester::_q_playback()
{
    m_stream->stopInput();
    _q_volumeChanged(0);

    // start output
    m_buffer->open(QIODevice::ReadOnly);
    m_buffer->reset();
    m_stream->startOutput(m_format, m_buffer);
    QTimer::singleShot(duration() * 1000, this, SLOT(_q_stop()));

    // update state
    m_state = PlayingState;
    emit stateChanged(m_state);
}

void QSoundTester::_q_stop()
{
    m_stream->stopOutput();
    _q_volumeChanged(0);

    // update state
    m_state = IdleState;
    emit stateChanged(m_state);
}

void QSoundTester::_q_volumeChanged(int volume)
{
    if (volume != m_volume) {
        m_volume = volume;
        emit volumeChanged(m_volume);
    }
}

