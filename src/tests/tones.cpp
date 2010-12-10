#include <cmath>

#include <QApplication>
#include <QAudioOutput>
#include <QFile>
#include <QLayout>
#include <QMap>
#include <QPair>
#include <QPushButton>

#include "QXmppRtpChannel.h"
#include "tones.h"

typedef QPair<int,int> ToneFreq;
static QMap<int, ToneFreq> tones;
static QList<ToneFreq> notes;

ToneGenerator::ToneGenerator(QObject *parent)
    : QIODevice(parent),
    m_clockrate(16000),
    m_clocktick(0),
    m_tone(-1),
    m_tonetick(0)
{
    if (tones.isEmpty()) {
        tones[QXmppRtpChannel::Tone_1] = qMakePair(697, 1209);
        tones[QXmppRtpChannel::Tone_2] = qMakePair(697, 1336);
        tones[QXmppRtpChannel::Tone_3] = qMakePair(697, 1477);
        tones[QXmppRtpChannel::Tone_A] = qMakePair(697, 1633);
        tones[QXmppRtpChannel::Tone_4] = qMakePair(770, 1209);
        tones[QXmppRtpChannel::Tone_5] = qMakePair(770, 1336);
        tones[QXmppRtpChannel::Tone_6] = qMakePair(770, 1477);
        tones[QXmppRtpChannel::Tone_B] = qMakePair(770, 1633);
        tones[QXmppRtpChannel::Tone_7] = qMakePair(852, 1209);
        tones[QXmppRtpChannel::Tone_8] = qMakePair(852, 1336);
        tones[QXmppRtpChannel::Tone_9] = qMakePair(852, 1477);
        tones[QXmppRtpChannel::Tone_C] = qMakePair(852, 1633);
        tones[QXmppRtpChannel::Tone_Star] = qMakePair(941, 1209);
        tones[QXmppRtpChannel::Tone_0] = qMakePair(941, 1336);
        tones[QXmppRtpChannel::Tone_Pound] = qMakePair(941, 1477);
        tones[QXmppRtpChannel::Tone_D] = qMakePair(941, 1633);
    }
}

int ToneGenerator::clockrate() const
{
    return m_clockrate;
}

void ToneGenerator::startTone(int tone)
{
    qDebug("start %i", tone);
    m_tone = tone;
    m_tonetick = m_clocktick;
}

void ToneGenerator::stopTone(int tone)
{
    qDebug("stop %i", tone);
    m_tone = -1;
}

qint64 ToneGenerator::readData(char * data, qint64 maxSize)
{
    if (m_tone < QXmppRtpChannel::Tone_0 || m_tone > QXmppRtpChannel::Tone_D) {
        memset(data, 0, maxSize);
        return maxSize;
    }

    ToneFreq tf = tones[m_tone];
    const qint16 *end = (qint16*)(data + maxSize);
    for (qint16 *ptr = (qint16*)data; ptr < end; ++ptr) {
        *ptr = 16383.0 * cos(float(2 * M_PI * m_clocktick * tf.first) / float(m_clockrate))
             + 16383.0 * cos(float(2 * M_PI * m_clocktick * tf.second) / float(m_clockrate));
        m_clocktick++;
    }
    return maxSize;
}

qint64 ToneGenerator::writeData(const char * data, qint64 maxSize)
{
    return maxSize;
}

ToneGui::ToneGui()
{
    // keyboard
    QGridLayout *grid = new QGridLayout;
    QStringList keys;
    keys << "1" << "2" << "3";
    keys << "4" << "5" << "6";
    keys << "7" << "8" << "9";
    keys << "*" << "0" << "#";
    for (int i = 0; i < keys.size(); ++i) {
        QPushButton *key = new QPushButton(keys[i]);
        connect(key, SIGNAL(pressed()), this, SLOT(keyPressed()));
        connect(key, SIGNAL(released()), this, SLOT(keyReleased()));
        grid->addWidget(key, i / 3, i % 3, 1, 1);
    }
    setLayout(grid);

    generator = new ToneGenerator(this);
    generator->open(QIODevice::Unbuffered | QIODevice::ReadOnly);

    QAudioFormat format;
    format.setChannelCount(1);
    format.setSampleRate(generator->clockrate());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioOutput *output = new QAudioOutput(format, this);
    output->setBufferSize(4096);
    output->start(generator);
}

static QXmppRtpChannel::Tone keyTone(QPushButton *key)
{
    char c = key->text()[0].toLatin1();
    if (c >= '0' && c <= '9')
        return QXmppRtpChannel::Tone(c - '0');
    else if (c == '*')
        return QXmppRtpChannel::Tone_Star;
    else if (c == '#')
        return QXmppRtpChannel::Tone_Pound;
}

void ToneGui::keyPressed()
{
    QPushButton *key = qobject_cast<QPushButton*>(sender());
    generator->startTone(keyTone(key));
}

void ToneGui::keyReleased()
{
    QPushButton *key = qobject_cast<QPushButton*>(sender());
    generator->stopTone(keyTone(key));
}

WavePlayer::WavePlayer(const QString &name, QObject *parent)
    : QIODevice(parent)
{
    m_file = new QFile(name, this);
}

QAudioFormat WavePlayer::format() const
{
    return m_format;
}

bool WavePlayer::open(QIODevice::OpenMode mode)
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

    qDebug("channelCount: %u, sampleRate: %u, sampleSize: %u", channelCount, sampleRate, sampleSize);

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

qint64 WavePlayer::readData(char * data, qint64 maxSize)
{
    char *start = data;
    while (maxSize) {
        qint64 chunk = qMin(m_endPos - m_file->pos(), maxSize);
        if (m_file->read(data, chunk) < 0)
            return -1;
        data += chunk;
        maxSize -= chunk;
        if (maxSize)
            m_file->seek(m_beginPos);
    }
    return data - start;
}

qint64 WavePlayer::writeData(const char * data, qint64 maxSize)
{
    return -1;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if 0
    if (argc < 2)
        return EXIT_FAILURE;

    WavePlayer player(QString::fromLocal8Bit(argv[1]));
    if (!player.open(QIODevice::Unbuffered | QIODevice::ReadOnly))
        return EXIT_FAILURE;

    QAudioOutput *output = new QAudioOutput(player.format());
    output->start(&player);
#else
    ToneGui gui;
    gui.show();
    gui.raise();
#endif

    return app.exec();
}

