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
#include <QByteArray>
#include <QFile>

#include "QSoundFile.h"
#ifdef USE_MAD
#include <mad.h>
#endif
#ifdef USE_VORBISFILE
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
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
    
    QString m_name;
    QAudioFormat m_format;
    QList<QPair<QSoundFile::MetaData, QString> > m_info;

    bool m_repeat;
};

QSoundFilePrivate::QSoundFilePrivate()
    : m_repeat(false)
{
}

#ifdef USE_MAD
class QSoundFileMp3 : public QSoundFilePrivate
{
public:
    QSoundFileMp3(QIODevice *device, QSoundFile *qq);
    void close();
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);


private:
    QSoundFile *q;

    bool decodeFrame();

    QIODevice *m_file;
    bool m_headerFound;
    QByteArray m_inputBuffer;
    QByteArray m_outputBuffer;

    mad_stream m_stream;
    mad_frame m_frame;
    mad_synth m_synth;
};

static inline qint16 mad_scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

QSoundFileMp3::QSoundFileMp3(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_file(device),
    m_headerFound(false)
{
    qDebug("Opening MP3 file");
}

void QSoundFileMp3::close()
{
    mad_synth_finish(&m_synth);
    mad_frame_finish(&m_frame);
    mad_stream_finish(&m_stream);
    m_inputBuffer.clear();
}

bool QSoundFileMp3::decodeFrame()
{
    if (mad_header_decode(&m_frame.header, &m_stream) == -1) {
        if (!MAD_RECOVERABLE(m_stream.error))
            return false;

        qWarning("Got an error while decoding MP3 header: 0x%04x (%s)\n", m_stream.error, mad_stream_errorstr(&m_stream));
        return true;
    }

    // process header
    if (!m_headerFound) {
        m_format.setChannels(MAD_NCHANNELS(&m_frame.header));
        m_format.setFrequency(m_frame.header.samplerate);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleSize(16);
        m_format.setSampleType(QAudioFormat::SignedInt);
        m_headerFound = true;
    }

    // process frame
    if (mad_frame_decode(&m_frame, &m_stream) == -1) {
        if (!MAD_RECOVERABLE(m_stream.error))
            return false;

        qWarning("Got an error while decoding MP3 frame: 0x%04x (%s)\n", m_stream.error, mad_stream_errorstr(&m_stream));
        return true;
    }
    mad_synth_frame(&m_synth, &m_frame);

    // write PCM
    QDataStream output(&m_outputBuffer, QIODevice::WriteOnly);
    output.device()->seek(m_outputBuffer.size());
    output.setByteOrder(QDataStream::LittleEndian);

    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;

    /* pcm->samplerate contains the sampling frequency */
    struct mad_pcm *pcm = &m_synth.pcm;
    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];

    while (nsamples--) {
        output << mad_scale(*left_ch++);
        if (nchannels == 2)
            output << mad_scale(*right_ch++);
    }

    return true;
}

static quint32 read_syncsafe_int(const QByteArray &ba)
{
    quint32 size = 0;
    size |= ba.at(0) << 21;
    size |= ba.at(1) << 14;
    size |= ba.at(2) << 7;
    size |= ba.at(3);
    return size;
}

bool QSoundFileMp3::open(QIODevice::OpenMode mode)
{
    if (mode & QIODevice::WriteOnly) {
        qWarning("Writing to MP3 is not supported");
        return false;
    }

    // read file contents
    if (!m_file->open(mode))
        return false;
    m_inputBuffer = m_file->readAll();
    m_file->close();

    // handle ID3 header
    qint64 pos = 0;
    if (m_inputBuffer.startsWith("ID3")) {
        // identifier + version
        pos += 5;

        // flags
        const quint8 flags = (quint8)(m_inputBuffer.at(pos));
#if 0
        qDebug("flags %x", flags);
        if (flags & 0x80) {
            qDebug("unsynchronisation");
        }
        if (flags & 0x40) {
            qDebug("extended header");
        }
        if (flags & 0x10) {
            qDebug("footer present");
        }
#endif
        pos++;

        // size
        const quint32 size = read_syncsafe_int(m_inputBuffer.mid(pos, 4));
        pos += 4;

        // contents
        QByteArray contents = m_inputBuffer.mid(pos, size);
        qint64 ptr = 0;
        while (ptr < contents.size()) {
            // frame header
            const QByteArray frameId = contents.mid(ptr, 4);
            ptr += 4;
            const quint32 frameSize = read_syncsafe_int(contents.mid(ptr, 4));
            ptr += 4;
            // const quint16 flags = ..
            ptr += 2;
            if (!frameSize)
                break;

            // frame content
            char enc = contents.at(ptr);
            const QByteArray ba = contents.mid(ptr+1, frameSize - 1);
            ptr += frameSize;

            QString value;
            switch (enc) {
            case 0:
                value = QString::fromLatin1(ba);
                break;
            case 3:
                value = QString::fromUtf8(ba);
                break;
            default:
                continue;
            }
            //qDebug("frame %s %s", frameId.constData(), qPrintable(value));

            if (frameId == "TPE1")
                m_info << qMakePair(QSoundFile::ArtistMetaData, value);
            else if (frameId == "TALB")
                m_info << qMakePair(QSoundFile::AlbumMetaData, value);
            else if (frameId == "TIT2")
                m_info << qMakePair(QSoundFile::TitleMetaData, value);
            else if (frameId == "TYER")
                m_info << qMakePair(QSoundFile::DateMetaData, value);
            else if (frameId == "TCON")
                m_info << qMakePair(QSoundFile::GenreMetaData, value);
            else if (frameId == "TRCK")
                m_info << qMakePair(QSoundFile::TracknumberMetaData, value);
            else if (frameId == "COMM")
                m_info << qMakePair(QSoundFile::DescriptionMetaData, value);
        }
        pos += size;
        m_inputBuffer = m_inputBuffer.mid(pos);
    }

    // skip any data preceding MP3 data
    int startPos = m_inputBuffer.indexOf("\xff\xfb");
    if (startPos > 0) {
        qDebug("Skipping %i non-MP3 bytes", startPos);
        m_inputBuffer = m_inputBuffer.mid(startPos);
    }

    // initialise decoder
    mad_stream_init(&m_stream);
    mad_frame_init(&m_frame);
    mad_synth_init(&m_synth);
    mad_stream_buffer(&m_stream, (unsigned char*)m_inputBuffer.data(), m_inputBuffer.size());

    // get first frame header
    if (!decodeFrame() || !m_headerFound)
        return false;

    return true;
}

void QSoundFileMp3::rewind()
{
    mad_stream_buffer(&m_stream, (unsigned char*)m_inputBuffer.data(), m_inputBuffer.size());
}

qint64 QSoundFileMp3::readData(char * data, qint64 maxSize)
{
    // decode frames
    while (m_outputBuffer.size() < maxSize) {
        if (!decodeFrame())
            break;
    }

    // copy output data
    qint64 bytes = qMin(maxSize, (qint64)m_outputBuffer.size());
    if (bytes > 0) {
        memcpy(data, m_outputBuffer.constData(), bytes);
        m_outputBuffer = m_outputBuffer.mid(bytes);
    }

    return bytes;
}

qint64 QSoundFileMp3::writeData(const char *data, qint64 maxSize)
{
    return -1;
}

#endif

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
    QSoundFileOgg(QIODevice *device, QSoundFile *qq);
    void close();
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QSoundFile *q;
    QIODevice *m_file;
    int m_section;
    OggVorbis_File m_vf;
};

QSoundFileOgg::QSoundFileOgg(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_file(device),
    m_section(0)
{
    qDebug("Opening OGG file");
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

    if (!m_file->open(mode))
        return false;

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
    QSoundFileWav(QIODevice *device, QSoundFile *qq);
    void close();
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    bool readHeader();
    bool writeHeader();

    QSoundFile *q;
    QIODevice *m_file;
    qint64 m_beginPos;
    qint64 m_endPos;
};

QSoundFileWav::QSoundFileWav(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_file(device),
    m_beginPos(0),
    m_endPos(0)
{
    qDebug("Opening WAV file");
}

bool QSoundFileWav::open(QIODevice::OpenMode mode)
{
    // open file
    if (!m_file->open(mode))
        return false;

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

                    if (key == "IART")
                        m_info << qMakePair(QSoundFile::ArtistMetaData, QString::fromUtf8(value));
                    else if (key == "INAM")
                        m_info << qMakePair(QSoundFile::TitleMetaData, QString::fromUtf8(value));
                    else if (key == "ICRD")
                        m_info << qMakePair(QSoundFile::DateMetaData, QString::fromUtf8(value));
                    else if (key == "IGNR")
                        m_info << qMakePair(QSoundFile::GenreMetaData, QString::fromUtf8(value));
                    else if (key == "ICMT")
                        m_info << qMakePair(QSoundFile::DescriptionMetaData, QString::fromUtf8(value));
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
        QPair<QSoundFile::MetaData, QString> info;
        foreach (info, m_info) {
            QByteArray key;
            switch (info.first) {
            case QSoundFile::ArtistMetaData:
                key = "IART";
                break;
            case QSoundFile::TitleMetaData:
                key = "INAM";
                break;
            case QSoundFile::DateMetaData:
                key = "ICRD";
                break;
            case QSoundFile::GenreMetaData:
                key = "IGNR";
                break;
            case QSoundFile::DescriptionMetaData:
                key = "ICMT";
                break;
            default:
                continue;
            }
            tmp.writeRawData(key.constData(), key.size());
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

static QSoundFilePrivate *factory(QIODevice *file, QSoundFile::FileType type, QSoundFile *parent)
{
    switch (type) {
    case QSoundFile::WavFile:
        return new QSoundFileWav(file, parent);
#ifdef USE_MAD
    case QSoundFile::Mp3File:
        return new QSoundFileMp3(file, parent);
#endif
#ifdef USE_VORBISFILE
    case QSoundFile::OggFile:
        return new QSoundFileOgg(file, parent);
#endif
    default:
        return 0;
    }
}

static QSoundFile::FileType guessMime(const QString &name)
{
    if (name.endsWith(".wav"))
        return QSoundFile::WavFile;
    else if (name.endsWith(".mp3"))
        return QSoundFile::Mp3File;
    else if (name.endsWith(".ogg"))
        return QSoundFile::OggFile;
    else
        return QSoundFile::UnknownFile;
}

/** Constructs a QSoundFile.
 *
 * @param file
 * @param type
 * @param parent
 */
QSoundFile::QSoundFile(QIODevice *file, FileType type, QObject *parent)
    : QIODevice(parent)
{
    file->setParent(this);
    d = factory(file, type, this);
}

/** Constructs a QSoundFile to represent the sound file with the given name.
 *
 * @param name
 * @param parent
 */
QSoundFile::QSoundFile(const QString &name, QObject *parent)
    : QIODevice(parent)
{
    QFile *file = new QFile(name, this);
    d = factory(file, guessMime(name), this);
    if (d)
        d->m_name = name;
}

/** Destroys the sound file object.
 */
QSoundFile::~QSoundFile()
{
    if (d)
        delete d;
}

/** Closes the sound file.
 */
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

QStringList QSoundFile::metaData(MetaData key) const
{
    QStringList values;
    if (d) {
        for (int i = 0; i < d->m_info.size(); ++i) {
            if (d->m_info[i].first == key)
                values << d->m_info[i].second;
        }
    }
    return values;
}

/** Returns the sound file meta-data.
 */
QList<QPair<QSoundFile::MetaData, QString> > QSoundFile::metaData() const
{
    if (d)
        return d->m_info;
    else
        return QList<QPair<QSoundFile::MetaData, QString> >();
}

/** Sets the sound file meta-data.
 *
 * @param info
 */
void QSoundFile::setMetaData(const QList<QPair<QSoundFile::MetaData, QString> > &info)
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
    if (!d->open(mode)) {
        qWarning("Could not open file %s", qPrintable(d->m_name));
        return false;
    }

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


