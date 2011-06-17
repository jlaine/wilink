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
#include "QSoundTester.h"

QSoundTester::QSoundTester(QObject *parent)
    : QObject(parent),
    m_input(0),
    m_output(0),
    m_state(Idle)
{
    m_buffer = new QBuffer(this);
}

int QSoundTester::duration() const
{
    return 5;
}

void QSoundTester::start(const QString &inputDeviceName, const QString &outputDeviceName)
{
    // find input device
    QAudioDeviceInfo inputDevice;
    bool found = false;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == inputDeviceName) {
            inputDevice = info;
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning("Could not find input device");
        return;
    }

    // found output device
    QAudioDeviceInfo outputDevice;
    found = false;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == outputDeviceName) {
            outputDevice = info;
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning("Could not find output device");
        return;
    }

    // prepare audio format
    QAudioFormat format;
    format.setFrequency(8000);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // create audio input and output
#ifdef Q_OS_MAC
    // 128ms at 8kHz
    const int bufferSize = 2048 * format.channels();
#else
    // 160ms at 8kHz
    const int bufferSize = 2560 * format.channels();
#endif
    m_input = new QAudioInput(inputDevice, format, this);
    m_input->setBufferSize(bufferSize);
    m_output = new QAudioOutput(outputDevice, format, this);
    m_output->setBufferSize(bufferSize);

    // start input
    m_buffer->open(QIODevice::WriteOnly);
    m_buffer->reset();
    QSoundMeter *inputMeter = new QSoundMeter(m_input->format(), m_buffer, m_input);
//    connect(inputMeter, SIGNAL(valueChanged(int)), testBar, SLOT(setValue(int)));
    m_input->start(inputMeter);
    QTimer::singleShot(duration() * 1000, this, SLOT(_q_playback()));

    // update state
    m_state = Recording;
    emit stateChanged(m_state);
}

QSoundTester::State QSoundTester::state() const
{
    return m_state;
}

void QSoundTester::_q_playback()
{
    // update state
    m_state = Playing;
    emit stateChanged(m_state);
}

void QSoundTester::_q_stop()
{
    // update state
    m_state = Idle;
    emit stateChanged(m_state);
}

