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

#include <cmath>

#include <QAudioFormat>
#include <QBuffer>
#include <QSysInfo>

#include "QSoundMeter.h"

static int loudness(const char *data, qint64 length, qint64 offset, int sampleSize)
{
    const char *ptr = data;
    int remainder = offset % sampleSize;
    if (remainder)
        ptr += (sampleSize - remainder);
    const char *end = data + length;
    remainder = (end - ptr) % sampleSize;
    if (remainder)
        end -= remainder;
    qint64 sum = 0;
    qint64 count = 0;
    for ( ; ptr < end; ptr += sampleSize) {
        qint16 sample = *((quint16*)ptr);
        sum += (sample * sample);
        count++;
    }
    return sqrt(sum/count);
}

QSoundMeter::QSoundMeter(const QAudioFormat &format, QObject *parent)
    : QIODevice(parent),
    m_device(0),
    m_value(0)
{
    if (format.byteOrder() != int(QSysInfo::ByteOrder) ||
        format.codec() != "audio/pcm" ||
        format.sampleSize() != 16) 
    {
        qWarning("QSoundMeter only supports 16-bit host-endian PCM data");
        return;
    }
    m_device = new QBuffer(this);
    m_device->open(QIODevice::ReadWrite);
    m_sampleSize = format.sampleSize() / 8;
    open(QIODevice::Unbuffered | QIODevice::ReadWrite);
}

int QSoundMeter::maximum() const
{
    return 32767;
}

qint64 QSoundMeter::pos() const
{
    if (!m_device)
        return 0;
    return m_device->pos();
}

qint64 QSoundMeter::readData(char *data, qint64 maxSize)
{
    if (!m_device)
        return -1;
    qint64 length = m_device->read(data, maxSize);
    if (length > 0) {
        const int newValue = loudness(data, length, m_readPos, m_sampleSize);
        if (newValue != m_value) {
            m_value = newValue;
            emit valueChanged(m_value);
        }
        m_readPos += length;
    }
    return length;
}

bool QSoundMeter::seek(qint64 pos)
{
    if (!m_device)
        return false;
    return m_device->seek(pos);
}

int QSoundMeter::value() const
{
    return m_value;
}

qint64 QSoundMeter::writeData(const char *data, qint64 maxSize)
{
    if (!m_device)
        return -1;
    qint64 length = m_device->write(data, maxSize);
    if (length > 0) {
        const int newValue = loudness(data, length, m_writePos, m_sampleSize);
        if (newValue != m_value) {
            m_value = newValue;
            emit valueChanged(m_value);
        }
        m_writePos += length;
    }
    return length;
}

