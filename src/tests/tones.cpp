#include <cmath>

#include <QApplication>
#include <QAudioOutput>
#include <QLayout>
#include <QMap>
#include <QPair>
#include <QPushButton>

#include "QXmppRtpChannel.h"

#include "qsound/QSoundPlayer.h"
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

    // buttons
    QHBoxLayout *hbox = new QHBoxLayout;
    QPushButton *startKey = new QPushButton("Start tone");
    connect(startKey, SIGNAL(pressed()), this, SLOT(startSound()));
    hbox->addWidget(startKey);
    QPushButton *stopKey = new QPushButton("Stop tone");
    connect(stopKey, SIGNAL(pressed()), this, SLOT(stopSound()));
    hbox->addWidget(stopKey);
    grid->addLayout(hbox, 4, 0, 1, 3);
    setLayout(grid);

    // generator
    generator = new ToneGenerator(this);
    generator->open(QIODevice::Unbuffered | QIODevice::ReadOnly);

    QAudioFormat format;
    format.setChannels(1);
    format.setFrequency(generator->clockrate());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioOutput *output = new QAudioOutput(format, this);
    output->setBufferSize(4096);
    output->start(generator);

    // player
    player = new QSoundPlayer(this);
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

void ToneGui::startSound()
{
    int soundId = player->play(":/tones.ogg", true);
    if (soundId >= 0)
        soundIds << soundId;
}

void ToneGui::stopSound()
{
    if (!soundIds.isEmpty())
        player->stop(soundIds.takeFirst());
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ToneGui gui;
    gui.show();
    gui.raise();

    return app.exec();
}

