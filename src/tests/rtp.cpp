#include <QDataStream>

#include "QXmppCodec.h"
#include "QXmppJingleIq.h"

#include "rtp.h"

#define QXMPP_DEBUG_RTP
#define SAMPLE_BYTES 2

const quint8 RTP_VERSION = 0x02;

class RtpChannelPrivate
{
public:
    RtpChannelPrivate();

    // signals
    bool signalsEmitted;
    qint64 writtenSinceLastEmit;

    // RTP
    QXmppCodec *codec;
    QHostAddress remoteHost;
    quint16 remotePort;

    QByteArray incomingBuffer;
    bool incomingBuffering;
    int incomingMinimum;
    int incomingMaximum;
    quint16 incomingSequence;
    quint32 incomingStamp;

    quint16 outgoingChunk;
    QByteArray outgoingBuffer;
    bool outgoingMarker;
    quint16 outgoingSequence;
    quint32 outgoingStamp;

    QXmppJinglePayloadType payloadType;
};

RtpChannelPrivate::RtpChannelPrivate()
    : signalsEmitted(false),
    writtenSinceLastEmit(0),
    codec(0),
    incomingBuffering(true),
    incomingMinimum(0),
    incomingMaximum(0),
    incomingSequence(0),
    incomingStamp(0),
    outgoingMarker(true),
    outgoingSequence(0),
    outgoingStamp(0)
{
}

RtpChannel::RtpChannel(QObject *parent)
    : QIODevice(parent),
    d(new RtpChannelPrivate)
{
}

RtpChannel::~RtpChannel()
{
    delete d;
}

/// Returns the number of bytes that are available for reading.
///

qint64 RtpChannel::bytesAvailable() const
{
    return d->incomingBuffer.size();
}

void RtpChannel::datagramReceived(const QByteArray &ba)
{
    if (!d->codec)
    {
        emit logMessage(QXmppLogger::WarningMessage,
                        QLatin1String("RtpChannel::datagramReceived before codec selection"));
        return;
    }

    if (ba.size() < 12 || (quint8(ba.at(0)) >> 6) != RTP_VERSION)
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QLatin1String("RtpChannel::datagramReceived got an invalid RTP packet"));
        return;
    }

    // parse RTP header
    QDataStream stream(ba);
    quint8 version, marker_type;
    quint32 ssrc;
    quint16 sequence;
    quint32 stamp;
    stream >> version;
    stream >> marker_type;
    stream >> sequence;
    stream >> stamp;
    stream >> ssrc;
    const bool marker = marker_type & 0x80;
    const quint8 type = marker_type & 0x7f;
    const qint64 packetLength = ba.size() - 12;

#ifdef QXMPP_DEBUG_RTP
    emit logMessage(QXmppLogger::ReceivedMessage,
        QString("RTP packet seq %1 stamp %2 marker %3 type %4 size %5").arg(
            QString::number(sequence),
            QString::number(stamp),
            QString::number(marker),
            QString::number(type),
            QString::number(packetLength)));
#endif

    // check type
    if (type != d->payloadType.id())
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QString("RTP packet seq %1 has unknown type %2")
                .arg(QString::number(sequence))
                .arg(QString::number(type)));
        return;
    }

    // check sequence number
    if (!marker && sequence != d->incomingSequence + 1)
        emit logMessage(QXmppLogger::WarningMessage,
            QString("RTP packet seq %1 is out of order, previous was %2")
                .arg(QString::number(sequence))
                .arg(QString::number(d->incomingSequence)));
    d->incomingSequence = sequence;

    // determine packet's position in the buffer (in bytes)
    qint64 packetOffset = 0;
    if (!d->incomingBuffer.isEmpty())
    {
        packetOffset = (stamp - d->incomingStamp) * SAMPLE_BYTES;
        if (packetOffset < 0)
        {
            emit logMessage(QXmppLogger::WarningMessage,
                QString("RTP packet stamp %1 is too old, buffer start is %2")
                    .arg(QString::number(stamp))
                    .arg(QString::number(d->incomingStamp)));
            return;
        }
    } else {
        d->incomingStamp = stamp;
    }

    // allocate space for new packet
    if (packetOffset + packetLength > d->incomingBuffer.size())
        d->incomingBuffer += QByteArray(packetOffset + packetLength - d->incomingBuffer.size(), 0);
    QDataStream output(&d->incomingBuffer, QIODevice::WriteOnly);
    output.device()->seek(packetOffset);
    output.setByteOrder(QDataStream::LittleEndian);
    d->codec->decode(stream, output);

    // check whether we are running late
    if (d->incomingBuffer.size() > d->incomingMaximum)
    {
        const qint64 droppedSize = d->incomingBuffer.size() - d->incomingMinimum;
        emit logMessage(QXmppLogger::DebugMessage,
            QString("RTP buffer is too full, dropping %1 bytes")
                .arg(QString::number(droppedSize)));
        d->incomingBuffer = d->incomingBuffer.right(d->incomingMinimum);
        d->incomingStamp += droppedSize / SAMPLE_BYTES;
    }
    // check whether we have filled the initial buffer
    if (d->incomingBuffer.size() >= d->incomingMinimum)
        d->incomingBuffering = false;
    if (!d->incomingBuffering)
        emit readyRead();
}

/// Returns true, as the RTP channel is a sequential device.
///

bool RtpChannel::isSequential() const
{
    return true;
}

qint64 RtpChannel::readData(char * data, qint64 maxSize)
{
    // if we are filling the buffer, return empty samples
    if (d->incomingBuffering)
    {
        memset(data, 0, maxSize);
        return maxSize;
    }

    qint64 readSize = qMin(maxSize, qint64(d->incomingBuffer.size()));
    memcpy(data, d->incomingBuffer.constData(), readSize);
    d->incomingBuffer.remove(0, readSize);
    if (readSize < maxSize)
    {
        emit logMessage(QXmppLogger::InformationMessage,
            QString("RtpChannel::readData missing %1 bytes").arg(QString::number(maxSize - readSize)));
        memset(data + readSize, 0, maxSize - readSize);
    }
    d->incomingStamp += readSize / SAMPLE_BYTES;
    return maxSize;
}

QXmppJinglePayloadType RtpChannel::payloadType() const
{
    return d->payloadType;
}

void RtpChannel::setPayloadType(const QXmppJinglePayloadType &payloadType)
{
    d->payloadType = payloadType;
    if (payloadType.id() == G711u)
        d->codec = new QXmppG711uCodec(payloadType.clockrate());
    else if (payloadType.id() == G711a)
        d->codec = new QXmppG711aCodec(payloadType.clockrate());
#ifdef QXMPP_USE_SPEEX
    else if (payloadType.name().toLower() == "speex")
        d->codec = new QXmppSpeexCodec(payloadType.clockrate());
#endif
    else
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QString("QXmppCall got an unknown codec : %1 (%2)")
                .arg(QString::number(payloadType.id()))
                .arg(payloadType.name()));
        return;
    }

    // size in bytes of an unencoded packet
    d->outgoingChunk = SAMPLE_BYTES * payloadType.ptime() * payloadType.clockrate() / 1000;

    // initial number of bytes to buffer
    d->incomingMinimum = d->outgoingChunk * 5;
    d->incomingMaximum = d->outgoingChunk * 8;

    open(QIODevice::ReadWrite | QIODevice::Unbuffered);
    //updateOpenMode();
}

qint64 RtpChannel::writeData(const char * data, qint64 maxSize)
{
    if (!d->codec)
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QLatin1String("RtpChannel::writeData before codec was set"));
        return -1;
    }

    d->outgoingBuffer += QByteArray::fromRawData(data, maxSize);
    while (d->outgoingBuffer.size() >= d->outgoingChunk)
    {
        QByteArray header;
        QDataStream stream(&header, QIODevice::WriteOnly);
        quint8 version = RTP_VERSION << 6;
        stream << version;
        quint8 marker_type = d->payloadType.id();
        if (d->outgoingMarker)
        {
            marker_type |= 0x80;
            d->outgoingMarker= false;
        }
        stream << marker_type;
        stream << ++d->outgoingSequence;
        stream << d->outgoingStamp;
        const quint32 ssrc = 0;
        stream << ssrc;

        QByteArray chunk = d->outgoingBuffer.left(d->outgoingChunk);
        QDataStream input(chunk);
        input.setByteOrder(QDataStream::LittleEndian);
        d->outgoingStamp += d->codec->encode(input, stream);

        // FIXME: write data
#ifdef QXMPP_DEBUG_RTP
        emit logMessage(QXmppLogger::SentMessage,
                        QString("RTP packet seq %1 stamp %2 marker %3 type %4 size %5").arg(
                                QString::number(d->outgoingSequence),
                                QString::number(d->outgoingStamp),
                                QString::number(marker_type & 0x80 != 0),
                                QString::number(marker_type & 0x7f),
                                QString::number(header.size() - 12)));
#endif
        emit sendDatagram(header);

        d->outgoingBuffer.remove(0, chunk.size());
    }

    d->writtenSinceLastEmit += maxSize;
    if (!d->signalsEmitted && !signalsBlocked()) {
        d->signalsEmitted = true;
        QMetaObject::invokeMethod(this, "emitSignals", Qt::QueuedConnection);
    }

    return maxSize;
}
