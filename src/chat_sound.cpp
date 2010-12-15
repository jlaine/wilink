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

static const quint32 RIFF_ID = 0x52494646;     // "RIFF"
static const quint32 RIFF_FORMAT = 0x57415645; // "WAVE"

static const quint32 FMT_ID = 0x666d7420; // "fmt "
static const quint32 FMT_SIZE = 16;
static const quint16 FMT_FORMAT = 1;      // linear PCM

static const quint32 DATA_ID = 0x64617461;

ChatSoundPlayer::ChatSoundPlayer(QObject *parent)
    : QObject(parent),
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

    QAudioOutput *output = new QAudioOutput(reader->format(), reader);
    output->start(reader);

    return m_readerId;
}

void ChatSoundPlayer::stop(int id)
{
    ChatSoundReader *reader = m_readers.value(id);
    if (reader)
        reader->close();
}

void ChatSoundPlayer::readerFinished()
{
    ChatSoundReader *reader = qobject_cast<ChatSoundReader*>(sender());
    if (!reader)
        return;

    int id = m_readers.key(reader);
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

void ChatSoundReader::close()
{
    if (!isOpen())
        return;

    QIODevice::close();
    m_file->close();
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

QAudioFormat ChatSoundReader::format() const
{
    return m_format;
}

bool ChatSoundReader::open(QIODevice::OpenMode mode)
{
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        qWarning("Cannot open in read/write mode");
        return false;
    }

    // open file
    if (!m_file->open(mode)) {
        qWarning("Could not open %s", qPrintable(m_file->fileName()));
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        // read header
        if (!readHeader()) {
            m_file->close();
            return false;
        }
    } 
    else if (mode & QIODevice::WriteOnly) {
        // write header
        if (!writeHeader()) {
            m_file->close();
            return false;
        }
    }

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
                close();
                //QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
                break;
            }
            m_file->seek(m_beginPos);
        }
    }
    return data - start;
}

bool ChatSoundReader::readHeader()
{
    QDataStream stream(m_file);

    // RIFF header
    quint32 chunkId, chunkSize, chunkFormat;
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkId;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> chunkSize;
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> chunkFormat;
    if (chunkId != RIFF_ID || chunkSize != m_file->size() - 8 || chunkFormat != RIFF_FORMAT) {
        qWarning("Bad RIFF header");
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
    if (chunkId != FMT_ID || chunkSize != FMT_SIZE || audioFormat != FMT_FORMAT) {
        qWarning("Bad fmt subchunk");
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
    if (chunkId != DATA_ID) {
        qWarning("Bad data subchunk");
        return false;
    }
    m_beginPos = m_file->pos();
    m_endPos = m_beginPos + chunkSize;

    // prepare format
    m_format.setChannels(channelCount);
    m_format.setFrequency(sampleRate);
    m_format.setSampleSize(sampleSize);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    return true;
}

qint64 ChatSoundReader::writeData(const char * data, qint64 maxSize)
{
    return -1;
}

bool ChatSoundReader::writeHeader()
{
    QDataStream stream(m_file);

    // RIFF header
    stream.setByteOrder(QDataStream::BigEndian);
    stream << RIFF_ID;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(m_file->size() - 8);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << RIFF_FORMAT;

    // fmt subchunk
    stream.setByteOrder(QDataStream::BigEndian);
    stream << FMT_ID;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << FMT_SIZE;
    stream << FMT_FORMAT;
    stream << quint16(m_format.channels());
    stream << quint32(m_format.frequency());
    stream << quint32((m_format.sampleSize() * m_format.frequency()) / 8);
    stream << quint16(0); //blockAlign;
    stream << quint16(m_format.sampleSize());

    // data subchunk
    stream.setByteOrder(QDataStream::BigEndian);
    stream << DATA_ID;
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(m_endPos - m_beginPos);

    return true;
}
