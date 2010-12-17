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
#include "speex/speex_header.h"

static const quint32 FMT_SIZE = 16;
static const quint16 FMT_FORMAT = 1;      // linear PCM

static const quint32 crc_lookup[256]={
  0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,
  0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
  0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,
  0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
  0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,
  0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
  0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,
  0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
  0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,
  0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
  0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,
  0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
  0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,
  0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
  0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,
  0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
  0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,
  0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
  0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,
  0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
  0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,
  0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
  0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,
  0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
  0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,
  0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
  0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,
  0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
  0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,
  0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
  0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,
  0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
  0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,
  0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
  0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,
  0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
  0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,
  0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
  0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,
  0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
  0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,
  0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
  0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,
  0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
  0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,
  0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
  0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,
  0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
  0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,
  0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
  0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,
  0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
  0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,
  0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
  0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,
  0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
  0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,
  0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
  0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,
  0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
  0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,
  0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
  0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,
  0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4};

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

class ChatSoundFilePrivate {
public:
    ChatSoundFilePrivate();
    virtual bool readHeader() = 0;
    virtual bool writeHeader() = 0;
    
    QAudioFormat m_format;
    QList<QPair<QByteArray, QString> > m_info;

    QFile *m_file;
    qint64 m_beginPos;
    qint64 m_endPos;
    bool m_repeat;
};

ChatSoundFilePrivate::ChatSoundFilePrivate()
    : m_beginPos(0),
    m_endPos(0),
    m_repeat(false)
{
}

class ChatSoundFileOgg : public ChatSoundFilePrivate {
public:
    bool readHeader();
    bool writeHeader();
};

class OggPage {
public:
    quint32 checksum() const;
    void dump();
    bool read(QDataStream &stream);
    bool write(QDataStream &stream) const;

    quint8 version;
    quint8 type;
    quint64 position;
    quint32 bitstream_sn;
    quint32 page_sn;
    QByteArray data;

private:
    QList<quint8> segments;
};

void OggPage::dump()
{
    qDebug("Page version %u, type %u", version, type);
    qDebug(" position: %llu", position);
    qDebug(" bitstream sn: %u", bitstream_sn);
    qDebug(" page sn: %u", page_sn);
    QStringList str;
    int pos = 0;
    foreach (quint8 length, segments) {
        str << QString("%1-%2").arg(QString::number(pos), QString::number(pos + length - 1));
        pos += length;
    }
    qDebug(" segments: %s", qPrintable(str.join(", ")));
}

quint32 OggPage::checksum() const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream.writeRawData("OggS", 4);
    stream << version;
    stream << type;
    stream << position;
    stream << bitstream_sn;
    stream << page_sn;
    stream << quint32(0);
    stream << quint8(segments.size());

    // write segment table
    for (int i = 0; i < segments.size(); ++i)
        stream << segments[i];

    // write segments
    stream.writeRawData(data.constData(), data.size());

    // calculate checksum
    const QByteArray bytes = buffer.data();
    quint32 crc_reg = 0;
    for (int i = 0; i < bytes.size(); ++i) {
      quint8 entry = (crc_reg >> 24) ^ bytes[i];
      crc_reg = (crc_reg << 8) ^ crc_lookup[entry];
    }

    quint8 *reg = (quint8*)(&crc_reg);
    return (reg[0] << 24) | (reg[1] << 16) | (reg[2] << 8) | reg[3];
}

bool OggPage::read(QDataStream &stream)
{
    char pattern[5];
    pattern[4] = '\0';
    quint8 page_segments;
    quint32 page_checksum;

    if (stream.readRawData(pattern, 4) != 4)
        return false;

    stream >> version;
    stream >> type;
    stream >> position;
    stream >> bitstream_sn;
    stream >> page_sn;
    stream >> page_checksum;
    stream >> page_segments;

    if (qstrcmp(pattern, "OggS") || version != 0) {
        qWarning("Bad OGG header");
        return false;
    }

    // read segment table
    quint8 length;
    quint16 total = 0;
    segments.clear();
    for (int i = 0; i < page_segments; ++i)
    {
        stream >> length;
        segments << length;
        total += length;
    }

    // read segments
    data.resize(total);
    if (stream.readRawData(data.data(), data.size()) != data.size())
        return false;

    // verify checksum
    quint32 calc_checksum = checksum();
    if (calc_checksum != page_checksum) {
        qWarning("Bad OGG page checksum");
        return false;
    }
    return true;
}

bool OggPage::write(QDataStream &stream) const
{
    return false;
}

bool ChatSoundFileOgg::readHeader()
{
    QDataStream stream(m_file);

    OggPage page;
    if (!page.read(stream))
        return false;
    page.dump();

    if (page.data.left(8) == QByteArray("Speex   ")) {
        // decode speex header
        QBuffer buffer(&page.data);
        buffer.open(QIODevice::ReadOnly);
        QDataStream data(&buffer);
        data.setByteOrder(QDataStream::LittleEndian);

        SpeexHeader h;
        data.readRawData(h.speex_string, sizeof(h.speex_string));
        data.readRawData(h.speex_version, sizeof(h.speex_version));
        data >> h.speex_version_id;
        data >> h.header_size;
        data >> h.rate;
        data >> h.mode;
        data >> h.mode_bitstream_version;
        data >> h.nb_channels;
        data >> h.bitrate;
        data >> h.frame_size;
        data >> h.vbr;
        data >> h.frames_per_packet;
        data >> h.extra_headers;
        data.skipRawData(8);
        buffer.close();

        qDebug("found speex %s", h.speex_version);
        m_format.setChannels(h.nb_channels);
        m_format.setFrequency(h.rate);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleSize(16);
        m_format.setSampleType(QAudioFormat::SignedInt);

        if (!page.read(stream))
            return false;

        page.dump();
        qDebug("truc: ", page.data.left(8).constData());
    } else {
        qWarning("Unsupported OGG payload");
        return false;
    }

    return true;
}

bool ChatSoundFileOgg::writeHeader()
{
    return false;
}

class ChatSoundFileWav : public ChatSoundFilePrivate {
public:
    bool readHeader();
    bool writeHeader();
};

bool ChatSoundFileWav::readHeader()
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

bool ChatSoundFileWav::writeHeader()
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

/** Constructs a ChatSoundFile.
 */
ChatSoundFile::ChatSoundFile(const QString &name, QObject *parent)
    : QIODevice(parent)
{
    if (name.endsWith(".ogg"))
        d = new ChatSoundFileOgg;
    else
        d = new ChatSoundFileWav;
    d->m_file = new QFile(name, this);
}

ChatSoundFile::~ChatSoundFile()
{
    delete d;
}

void ChatSoundFile::close()
{
    if (!isOpen())
        return;

    if (openMode() & QIODevice::WriteOnly) {
        d->m_file->seek(0);
        d->writeHeader();
    }
    d->m_file->close();
    QIODevice::close();
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

/** Returns the sound file format.
 */
QAudioFormat ChatSoundFile::format() const
{
    return d->m_format;
}

/** Sets the sound file format.
 *
 * @param format
 */
void ChatSoundFile::setFormat(const QAudioFormat &format)
{
    d->m_format = format;
}

/** Returns the sound file meta-data.
 */
QList<QPair<QByteArray, QString> > ChatSoundFile::info() const
{
    return d->m_info;
}

/** Sets the sound file meta-data.
 *
 * @param info
 */
void ChatSoundFile::setInfo(const QList<QPair<QByteArray, QString> > &info)
{
    d->m_info = info;
}

bool ChatSoundFile::open(QIODevice::OpenMode mode)
{
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        qWarning("Cannot open in read/write mode");
        return false;
    }

    // open file
    if (!d->m_file->open(mode)) {
        qWarning("Could not open %s", qPrintable(d->m_file->fileName()));
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        // read header
        if (!d->readHeader()) {
            d->m_file->close();
            return false;
        }
    } 
    else if (mode & QIODevice::WriteOnly) {
        // write header
        if (!d->writeHeader()) {
            d->m_file->close();
            return false;
        }
    }

    return QIODevice::open(mode);
}

qint64 ChatSoundFile::readData(char * data, qint64 maxSize)
{
    char *start = data;

    while (maxSize) {
        qint64 chunk = qMin(d->m_endPos - d->m_file->pos(), maxSize);
        qint64 bytes = d->m_file->read(data, chunk);
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
            d->m_file->seek(d->m_beginPos);
        }
    }
    return data - start;
}

/** Returns true if the file should be read repeatedly.
 */
bool ChatSoundFile::repeat() const
{
    return d->m_repeat;
}

/** Set to true if the file should be read repeatedly.
 *
 * @param repeat
 */
void ChatSoundFile::setRepeat(bool repeat)
{
    d->m_repeat = repeat;
}

qint64 ChatSoundFile::writeData(const char * data, qint64 maxSize)
{
    qint64 bytes = d->m_file->write(data, maxSize);
    if (bytes > 0)
        d->m_endPos += bytes;
    return bytes;
}


