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

#include "QSoundFile.h"
#ifdef USE_VORBISFILE
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#endif

static const quint32 FMT_SIZE = 16;
static const quint16 FMT_FORMAT = 1;      // linear PCM

class QSoundFilePrivate {
public:
    QSoundFilePrivate();
    virtual void close() = 0;
    virtual bool open(QIODevice::OpenMode mode) = 0;
    virtual void rewind() = 0;
    virtual qint64 readData(char * data, qint64 maxSize) = 0;
    virtual qint64 writeData(const char *data, qint64 maxSize) = 0;
    
    QAudioFormat m_format;
    QList<QPair<QByteArray, QString> > m_info;

    bool m_repeat;
};

QSoundFilePrivate::QSoundFilePrivate()
    : m_repeat(false)
{
}

#ifdef USE_VORBISFILE
static size_t qfile_read_callback(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    return file->read((char*)ptr, size * nmemb);
}

static int qfile_seek_callback(void *datasource, ogg_int64_t offset, int whence)
{
    QFile *file = static_cast<QFile*>(datasource);
    qint64 pos = offset;
    if (whence == SEEK_CUR)
        pos += file->pos();
    else if (whence == SEEK_END)
        pos += file->size();
    return file->seek(pos) ? 0 : -1;
}

static int qfile_close_callback(void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    file->close();
    return 0;
}

static long qfile_tell_callback(void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    return file->pos();
}

class QSoundFileOgg : public QSoundFilePrivate
{
public:
    QSoundFileOgg(const QString &name, QSoundFile *qq);
    void close();
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QSoundFile *q;
    QFile *m_file;
    QString m_name;
    int m_section;
    OggVorbis_File m_vf;
};

QSoundFileOgg::QSoundFileOgg(const QString &name, QSoundFile *qq)
    : q(qq),
    m_name(name),
    m_section(0)
{
    m_file = new QFile(name, q);
}

void QSoundFileOgg::close()
{
    ov_clear(&m_vf);
}

bool QSoundFileOgg::open(QIODevice::OpenMode mode)
{
    if (mode & QIODevice::WriteOnly) {
        qWarning("Writing to OGG is not supported");
        return false;
    }

    if (!m_file->open(mode)) {
        qWarning("Could not open %s", qPrintable(m_file->fileName()));
        return false;
    }

    ov_callbacks callbacks;
    callbacks.read_func = qfile_read_callback;
    callbacks.seek_func = qfile_seek_callback;
    callbacks.close_func = qfile_close_callback;
    callbacks.tell_func = qfile_tell_callback;
    if (ov_open_callbacks(m_file, &m_vf, 0, 0, callbacks) < 0) {
        qWarning("Input does not appear to be an Ogg bitstream");
        return false;
    }

    vorbis_info *vi = ov_info(&m_vf, -1);
    m_format.setChannels(vi->channels);
    m_format.setFrequency(vi->rate);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleSize(16);
    m_format.setSampleType(QAudioFormat::SignedInt);

    return true;
}

qint64 QSoundFileOgg::readData(char * data, qint64 maxSize)
{
    char *ptr = data;
    while (maxSize) {
        qint64 bytes = ov_read(&m_vf, ptr, maxSize, 0, 2, 1, &m_section);
        if (bytes < 0)
            return -1;
        else if (!bytes)
            break;
        ptr += bytes;
        maxSize -= bytes;
    }
    return ptr - data;
}

void QSoundFileOgg::rewind()
{
    ov_raw_seek(&m_vf, 0);
}

qint64 QSoundFileOgg::writeData(const char * data, qint64 maxSize)
{
    return -1;
}
#endif

class QSoundFileWav : public QSoundFilePrivate
{
public:
    QSoundFileWav(const QString &name, QSoundFile *qq);
    void close();
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    bool readHeader();
    bool writeHeader();

    QSoundFile *q;
    QFile *m_file;
    qint64 m_beginPos;
    qint64 m_endPos;
};

QSoundFileWav::QSoundFileWav(const QString &name, QSoundFile *qq)
    : q(qq),
    m_beginPos(0),
    m_endPos(0)
{
    m_file = new QFile(name, q);
}

bool QSoundFileWav::open(QIODevice::OpenMode mode)
{
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
    return true;
}

void QSoundFileWav::close()
{
    if (q->openMode() & QIODevice::WriteOnly) {
        m_file->seek(0);
        writeHeader();
    }
    m_file->close();
}

qint64 QSoundFileWav::readData(char * data, qint64 maxSize)
{
    return m_file->read(data, maxSize);
}

bool QSoundFileWav::readHeader()
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
                    int pos = value.indexOf('\0');
                    if (pos >= 0)
                        value = value.left(pos);
                    m_info << qMakePair(key, QString::fromUtf8(value));
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

void QSoundFileWav::rewind()
{
    m_file->seek(m_beginPos);
}

qint64 QSoundFileWav::writeData(const char * data, qint64 maxSize)
{
    qint64 bytes = m_file->write(data, maxSize);
    if (bytes > 0)
        m_endPos += bytes;
    return bytes;
}

bool QSoundFileWav::writeHeader()
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
        QPair<QByteArray, QString> info;
        foreach (info, m_info) {
            tmp.writeRawData(info.first.constData(), info.first.size());
            QByteArray value = info.second.toUtf8() + '\0';
            if (value.size() % 2)
                value += '\0';
            tmp << quint32(value.size());
            tmp.writeRawData(value.constData(), value.size());
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

/** Constructs a QSoundFile.
 */
QSoundFile::QSoundFile(const QString &name, QObject *parent)
    : QIODevice(parent)
{
    if (name.endsWith(".wav"))
        d = new QSoundFileWav(name, this);
#ifdef USE_VORBISFILE
    else if (name.endsWith(".ogg"))
        d = new QSoundFileOgg(name, this);
#endif
    else
        d = 0;
}

QSoundFile::~QSoundFile()
{
    if (d)
        delete d;
}

void QSoundFile::close()
{
    if (!isOpen())
        return;

    d->close();
    QIODevice::close();
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

/** Returns the sound file format.
 */
QAudioFormat QSoundFile::format() const
{
    if (d)
        return d->m_format;
    else
        return QAudioFormat();
}

/** Sets the sound file format.
 *
 * @param format
 */
void QSoundFile::setFormat(const QAudioFormat &format)
{
    if (d)
        d->m_format = format;
}

/** Returns the sound file meta-data.
 */
QList<QPair<QByteArray, QString> > QSoundFile::info() const
{
    if (d)
        return d->m_info;
    else
        return QList<QPair<QByteArray, QString> >();
}

/** Sets the sound file meta-data.
 *
 * @param info
 */
void QSoundFile::setInfo(const QList<QPair<QByteArray, QString> > &info)
{
    if (d)
        d->m_info = info;
}

bool QSoundFile::open(QIODevice::OpenMode mode)
{
    if (!d) {
        qWarning("Unsupported file type");
        return false;
    }

    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        qWarning("Cannot open in read/write mode");
        return false;
    }

    // open file
    if (!d->open(mode))
        return false;

    return QIODevice::open(mode);
}

qint64 QSoundFile::readData(char * data, qint64 maxSize)
{
    char *start = data;

    while (maxSize) {
        qint64 bytes = d->readData(data, maxSize);
        if (bytes < 0) {
            // abort
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
            return -1;
        }
        data += bytes;
        maxSize -= bytes;

        if (maxSize) {
            if (!d->m_repeat) {
                // stop, we have reached end of file and don't want to repeat
                memset(data, 0, maxSize);
                QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
                break;
            }
            // rewind file to repeat
            d->rewind();
        }
    }
    return data - start;
}

/** Returns true if the file should be read repeatedly.
 */
bool QSoundFile::repeat() const
{
    if (d)
        return d->m_repeat;
    else
        return false;
}

/** Set to true if the file should be read repeatedly.
 *
 * @param repeat
 */
void QSoundFile::setRepeat(bool repeat)
{
    if (d)
        d->m_repeat = repeat;
}

qint64 QSoundFile::writeData(const char * data, qint64 maxSize)
{
    return d->writeData(data, maxSize);
}


