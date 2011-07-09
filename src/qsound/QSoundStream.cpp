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
#include <QTime>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"

static int bufferFor(const QAudioFormat &format)
{
#ifdef Q_OS_MAC
    // 128ms at 8kHz
    return 2048 * format.channels();
#else
    // 160ms at 8kHz
    return 2560 * format.channels();
#endif
}

QSoundStream::QSoundStream(QSoundPlayer *player)
    : m_audioInput(0),
    m_audioInputMeter(0),
    m_audioOutput(0),
    m_audioOutputMeter(0),
    m_soundPlayer(player)
{
}

/** Starts audio capture.
 */
void QSoundStream::startInput(const QAudioFormat &format, QIODevice *device)
{
    bool check;
    Q_UNUSED(check);

    if (!m_audioInput) {
        const int bufferSize = bufferFor(format);

        QTime tm;
        tm.start();

        m_audioInput = new QAudioInput(m_soundPlayer->inputDevice(), format, this);
        check = connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(_q_audioInputStateChanged()));
        Q_ASSERT(check);

        m_audioInputMeter = new QSoundMeter(format, device, this);
        check = connect(m_audioInputMeter, SIGNAL(valueChanged(int)),
                        this, SIGNAL(inputVolumeChanged(int)));
        Q_ASSERT(check);

        m_audioInput->setBufferSize(bufferSize);
        m_audioInput->start(m_audioInputMeter);

        qDebug("Audio input initialized in %i ms", tm.elapsed());
        qDebug("Audio input buffer size %i (asked for %i)", m_audioInput->bufferSize(), bufferSize);
    }
}

/** Stops audio capture.
 */
void QSoundStream::stopInput()
{
    if (m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = 0;
        delete m_audioInputMeter;
        m_audioInputMeter = 0;

        emit inputVolumeChanged(0);
    }
}

/** Starts audio playback.
 */
void QSoundStream::startOutput(const QAudioFormat &format, QIODevice *device)
{
    bool check;
    Q_UNUSED(check);

    if (!m_audioOutput) {
        const int bufferSize = bufferFor(format);

        QTime tm;
        tm.start();

        m_audioOutput = new QAudioOutput(m_soundPlayer->outputDevice(), format, this);
        check = connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(_q_audioOutputStateChanged()));
        Q_ASSERT(check);

        m_audioOutputMeter = new QSoundMeter(format, device, this);
        check = connect(m_audioOutputMeter, SIGNAL(valueChanged(int)),
                        this, SIGNAL(outputVolumeChanged(int)));
        Q_ASSERT(check);

        m_audioOutput->setBufferSize(bufferSize);
        m_audioOutput->start(m_audioOutputMeter);

        qDebug("Audio output initialized in %i ms", tm.elapsed());
        qDebug("Audio output buffer size %i (asked for %i)", m_audioOutput->bufferSize(), bufferSize);
    }
}

/** Stops audio playback.
 */
void QSoundStream::stopOutput()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = 0;
        delete m_audioOutputMeter;
        m_audioOutputMeter = 0;

        outputVolumeChanged(0);
    }
}

int QSoundStream::inputVolume() const
{
    return m_audioInputMeter ? m_audioInputMeter->value() : 0;
}

int QSoundStream::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int QSoundStream::outputVolume() const
{
    return m_audioOutputMeter ? m_audioOutputMeter->value() : 0;
}

void QSoundStream::_q_audioInputStateChanged()
{
#if 0
    qDebug("Audio input state %i error %i",
           m_audioInput->state(),
           m_audioInput->error());
#endif

    // Restart audio input if we get an underrun.
    //
    // NOTE: seen on Mac OS X 10.6
    if (m_audioInput->state() == QAudio::IdleState &&
            m_audioInput->error() == QAudio::UnderrunError) {
        qWarning("Audio input needs restart due to buffer underrun");
        m_audioInput->start(m_audioInputMeter);
    }
}

void QSoundStream::_q_audioOutputStateChanged()
{
#if 0
    qDebug("Audio output state %i error %i",
           m_audioOutput->state(),
           m_audioOutput->error());
#endif

    // Restart audio output if we get an underrun.
    //
    // NOTE: seen on Linux with pulseaudio
    if (m_audioOutput->state() == QAudio::IdleState &&
            m_audioOutput->error() == QAudio::UnderrunError) {
        qWarning("Audio output needs restart due to buffer underrun");
        m_audioOutput->start(m_audioOutputMeter);
    }
}
