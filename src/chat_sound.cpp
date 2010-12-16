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
#include <QBuffer>
#include <QFile>

#include "chat_sound.h"

static const quint32 FMT_SIZE = 16;
static const quint16 FMT_FORMAT = 1;      // linear PCM

ChatSoundPlayer::ChatSoundPlayer(QObject *parent)
    : QObject(parent),
    m_readerId(0)
{
}

int ChatSoundPlayer::play(const QString &name, bool repeat)
{
    ChatSoundFile *reader = new ChatSoundFile(name, this);
    reader->setRepeat(repeat);
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
    ChatSoundFile *reader = m_readers.value(id);
    if (reader)
        reader->close();
}

void ChatSoundPlayer::readerFinished()
{
    ChatSoundFile *reader = qobject_cast<ChatSoundFile*>(sender());
    if (!reader)
        return;

    int id = m_readers.key(reader);
    m_readers.take(id);
    reader->deleteLater();
}

/** Constructs a ChatSoundFile.
 */
ChatSoundFile::ChatSoundFile(const QString &name, QObject *parent)
    : QIODevice(parent),
    m_beginPos(0),
    m_endPos(0),
    m_repeat(false)
{
    m_file = new QFile(name, this);
}

void ChatSoundFile::close()
{
    if (!isOpen())
        return;

    if (openMode() & QIODevice::WriteOnly) {
        m_file->seek(0);
        writeHeader();
    }
    m_file->close();
    QIODevice::close();
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

/** Returns the sound file format.
 */
QAudioFormat ChatSoundFile::format() const
{
    return m_format;
}

/** Sets the sound file format.
 *
 * @param format
 */
void ChatSoundFile::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

/** Returns the sound file meta-data.
 */
QList<QPair<QByteArray, QByteArray> > ChatSoundFile::info() const
{
    return m_info;
}

/** Sets the sound file meta-data.
 *
 * @param info
 */
void ChatSoundFile::setInfo(const QList<QPair<QByteArray, QByteArray> > &info)
{
    m_info = info;
}

bool ChatSoundFile::open(QIODevice::OpenMode mode)
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

qint64 ChatSoundFile::readData(char * data, qint64 maxSize)
{
    char *start = data;

    while (maxSize) {
        qint64 chunk = qMin(m_endPos - m_file->pos(), maxSize);
        qint64 bytes = m_file->read(data, chunk);
        if (bytes < 0) {
            // abort
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
            return -1;
        }
        data += bytes;
        maxSize -= bytes;

        if (maxSize) {
            if (!m_repeat) {
                // stop, we have reached end of file and don't want to repeat
                memset(data, 0, maxSize);
                QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
                break;
            }
            // rewind file to repeat
            m_file->seek(m_beginPos);
        }
    }
    return data - start;
}

bool ChatSoundFile::readHeader()
{
    QDataStream stream(m_file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char chunkId[5];
    chunkId[4] = '\0';
    quint32 chunkSize;
    char chunkFormat[5];
    chunkFormat[4] = '\0';

    // RIFF header
    if (stream.readRawData(chunkId, 4) != 4)
        return false;
    stream >> chunkSize;

    stream.readRawData(chunkFormat, 4);
    if (qstrcmp(chunkId, "RIFF") || chunkSize != m_file->size() - 8 || qstrcmp(chunkFormat, "WAVE")) {
        qWarning("Bad RIFF header");
        return false;
    }

    // read subchunks
    m_info.clear();
    while (true) {
        if (stream.readRawData(chunkId, 4) != 4)
            break;
        stream >> chunkSize;

        //qDebug("subchunk %s (%u bytes)", chunkId, chunkSize);
        if (!qstrcmp(chunkId, "fmt ")) {
            // fmt subchunk
            quint16 audioFormat, channelCount, blockAlign, sampleSize;
            quint32 sampleRate, byteRate;
            stream >> audioFormat;
            if (chunkSize != FMT_SIZE || audioFormat != FMT_FORMAT) {
                qWarning("Bad fmt subchunk");
                return false;
            }
            stream >> channelCount;
            stream >> sampleRate;
            stream >> byteRate;
            stream >> blockAlign;
            stream >> sampleSize;

            //qDebug("channelCount: %u, sampleRate: %u, sampleSize: %u", channelCount, sampleRate, sampleSize);
            // prepare format
            m_format.setChannels(channelCount);
            m_format.setFrequency(sampleRate);
            m_format.setSampleSize(sampleSize);
            m_format.setCodec("audio/pcm");
            m_format.setByteOrder(QAudioFormat::LittleEndian);
            m_format.setSampleType(QAudioFormat::SignedInt);
        } else if (!qstrcmp(chunkId, "data")) {
            // data subchunk
            m_beginPos = m_file->pos();
            m_endPos = m_beginPos + chunkSize;
            break;
        } else if (!qstrcmp(chunkId, "LIST")) {
            stream.readRawData(chunkFormat, 4);
            chunkSize -= 4;
            if (!qstrcmp(chunkFormat, "INFO")) {
                QByteArray key(4, '\0');
                QByteArray value;
                quint32 length;
                while (chunkSize) {
                    stream.readRawData(key.data(), key.size());
                    stream >> length;
                    value.resize(length);
                    stream.readRawData(value.data(), value.size());
                    m_info << qMakePair(key, value);
                    chunkSize -= (8 + length);
                }
            } else {
                qWarning("WAV file contains an unknown LIST format %s", chunkFormat);
                stream.skipRawData(chunkSize);
            }
        } else {
            // skip unknown subchunk
            qWarning("WAV file contains unknown subchunk %s", chunkId);
            stream.skipRawData(chunkSize);
        }
    }
    return true;
}

/** Returns true if the file should be read repeatedly.
 */
bool ChatSoundFile::repeat() const
{
    return m_repeat;
}

/** Set to true if the file should be read repeatedly.
 *
 * @param repeat
 */
void ChatSoundFile::setRepeat(bool repeat)
{
    m_repeat = repeat;
}

qint64 ChatSoundFile::writeData(const char * data, qint64 maxSize)
{
    qint64 bytes = m_file->write(data, maxSize);
    if (bytes > 0)
        m_endPos += bytes;
    return bytes;
}

bool ChatSoundFile::writeHeader()
{
    QDataStream stream(m_file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // RIFF header
    stream.writeRawData("RIFF", 4);
    stream << quint32(m_endPos - 8);
    stream.writeRawData("WAVE", 4);

    // fmt subchunk
    stream.writeRawData("fmt ", 4);
    stream << FMT_SIZE;
    stream << FMT_FORMAT;
    stream << quint16(m_format.channels());
    stream << quint32(m_format.frequency());
    stream << quint32((m_format.channels() * m_format.sampleSize() * m_format.frequency()) / 8);
    stream << quint16((m_format.channels() * m_format.sampleSize()) / 8);
    stream << quint16(m_format.sampleSize());

    // LIST subchunk
    if (!m_info.isEmpty()) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream tmp(&buffer);
        tmp.setByteOrder(QDataStream::LittleEndian);
        QPair<QByteArray, QByteArray> info;
        foreach (info, m_info) {
            tmp.writeRawData(info.first.constData(), info.first.size());
            tmp << quint32(info.second.size());
            tmp.writeRawData(info.second.constData(), info.second.size());
        }
        const QByteArray data = buffer.data();
        stream.writeRawData("LIST", 4);
        stream << quint32(4 + data.size());
        stream.writeRawData("INFO", 4);
        stream.writeRawData(data.constData(), data.size());
    }
    // data subchunk
    stream.writeRawData("data", 4);
    if (m_beginPos != m_file->pos() + 4)
        m_beginPos = m_endPos = m_file->pos() + 4;
    stream << quint32(m_endPos - m_beginPos);

    return true;
}
