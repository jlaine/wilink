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

#include <QAudioOutput>
#include <QFile>

#include "chat_sound.h"

ChatSoundPlayer::ChatSoundPlayer(QObject *parent)
    : QObject(parent),
    m_output(0),
    m_readerId(0)
{
}

int ChatSoundPlayer::play(const QString &name, int repeat)
{
    ChatSoundReader *reader = new ChatSoundReader(name, repeat, this);
    if (!reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        delete reader;
        return -1;
    }
    connect(reader, SIGNAL(finished()), this, SLOT(readerFinished()));
    m_readerId++;
    m_readers[m_readerId] = reader;

    m_output = new QAudioOutput(reader->format());
    m_output->start(reader);

    return m_readerId;
}

void ChatSoundPlayer::stop(int id)
{
    ChatSoundReader *reader = m_readers.value(id);
    if (reader)
        emit reader->finished();
}

void ChatSoundPlayer::readerFinished()
{
    ChatSoundReader *reader = qobject_cast<ChatSoundReader*>(sender());
    if (!reader)
        return;

    int id = m_readers.key(reader);
    if (id == m_readerId) {
        m_output->stop();
        m_readerId = 0;
    }
    m_readers.take(id);
    reader->deleteLater();
}

ChatSoundReader::ChatSoundReader(const QString &name, int repeat, QObject *parent)
    : QIODevice(parent),
    m_repeatCount(repeat),
    m_repeatLeft(repeat)
{
    m_file = new QFile(name, this);
}

QAudioFormat ChatSoundReader::format() const
{
    return m_format;
}

bool ChatSoundReader::open(QIODevice::OpenMode mode)
{
    if (!m_file->open(QIODevice::ReadOnly)) {
        qWarning("Could not read %s", qPrintable(m_file->fileName()));
        return false;
    }

    QDataStream stream(m_file);

    // RIFF header
    quint32 chunkId, chunkSize, chunkFormat;
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkId;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> chunkSize;
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkFormat;
    if (chunkId != 0x52494646 || chunkSize != m_file->size() - 8 || chunkFormat != 0x57415645) {
        qWarning("Bad RIFF header");
        m_file->close();
        return false;
    }

    // fmt subchunk
    quint16 audioFormat, channelCount, blockAlign, sampleSize;
    quint32 sampleRate, byteRate;
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkId;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> chunkSize;
    stream >> audioFormat;
    if (chunkId != 0x666d7420 || chunkSize != 16 || audioFormat != 1) {
        qWarning("Bad fmt subchunk");
        m_file->close();
        return false;
    }
    stream >> channelCount;
    stream >> sampleRate;
    stream >> byteRate;
    stream >> blockAlign;
    stream >> sampleSize;

    //qDebug("channelCount: %u, sampleRate: %u, sampleSize: %u", channelCount, sampleRate, sampleSize);

    // data subchunk
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkId;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> chunkSize;
    if (chunkId != 0x64617461) {
        qWarning("Bad data subchunk");
        return false;
    }
    m_beginPos = m_file->pos();
    m_endPos = m_beginPos + chunkSize;

    // prepare format
    m_format.setChannelCount(channelCount);
    m_format.setSampleRate(sampleRate);
    m_format.setSampleSize(sampleSize);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    return QIODevice::open(mode);
}

qint64 ChatSoundReader::readData(char * data, qint64 maxSize)
{
    char *start = data;

    // if we have finished playing, return empty samples
    if (m_repeatCount && !m_repeatLeft) {
        memset(data, 0, maxSize);
        return maxSize;
    }

    while (maxSize) {
        qint64 chunk = qMin(m_endPos - m_file->pos(), maxSize);
        qint64 bytes = m_file->read(data, chunk);
        if (bytes < 0)
            return -1;
        data += bytes;
        maxSize -= bytes;

        if (maxSize) {
            if (m_repeatCount && !--m_repeatLeft) {
                memset(data, 0, maxSize);
                QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
                break;
            }
            m_file->seek(m_beginPos);
        }
    }
    return data - start;
}

qint64 ChatSoundReader::writeData(const char * data, qint64 maxSize)
{
    return -1;
}

