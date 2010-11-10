#include <QDataStream>

#include "QXmppCodec.h"

#include "rtp.h"

const quint8 RTP_VERSION = 0x02;
#define SAMPLE_BYTES 2

RtpChannel::RtpChannel(QObject *parent)
    : QIODevice(parent),
    m_signalsEmitted(false),
    m_writtenSinceLastEmit(0),
    m_codec(0),
    m_incomingBuffering(true),
    m_incomingMinimum(0),
    m_incomingMaximum(0),
    m_incomingSequence(0),
    m_incomingStamp(0),
    m_outgoingMarker(true),
    m_outgoingSequence(0),
    m_outgoingStamp(0)
{
}

void RtpChannel::datagramReceived(const QByteArray &buffer)
{
    if (!m_codec)
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QLatin1String("QXmppCall:datagramReceived before codec selection"));
        return;
    }

    if (buffer.size() < 12 || (quint8(buffer.at(0)) >> 6) != RTP_VERSION)
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QLatin1String("QXmppCall::datagramReceived got an invalid RTP packet"));
        return;
    }

    // parse RTP header
    QDataStream stream(buffer);
    quint8 version, type;
    quint32 ssrc;
    quint16 sequence;
    quint32 stamp;
    stream >> version;
    stream >> type;
    stream >> sequence;
    stream >> stamp;
    stream >> ssrc;
    const qint64 packetLength = buffer.size() - 12;

#ifdef QXMPP_DEBUG_RTP
    emit logMessage(QXmppLogger::ReceivedMessage,
        QString("RTP packet seq %1 stamp %2 size %3")
            .arg(QString::number(sequence))
            .arg(QString::number(stamp))
            .arg(QString::number(packetLength)));
#endif

    // check sequence number
    if (sequence != m_incomingSequence + 1)
        emit logMessage(QXmppLogger::WarningMessage,
            QString("RTP packet seq %1 is out of order, previous was %2")
                .arg(QString::number(sequence))
                .arg(QString::number(m_incomingSequence)));
    m_incomingSequence = sequence;

    // determine packet's position in the buffer (in bytes)
    qint64 packetOffset = 0;
    if (!buffer.isEmpty())
    {
        packetOffset = (stamp - m_incomingStamp) * SAMPLE_BYTES;
        if (packetOffset < 0)
        {
            emit logMessage(QXmppLogger::WarningMessage,
                QString("RTP packet stamp %1 is too old, buffer start is %2")
                    .arg(QString::number(stamp))
                    .arg(QString::number(m_incomingStamp)));
            return;
        }
    } else {
        m_incomingStamp = stamp;
    }

    // allocate space for new packet
    if (packetOffset + packetLength > m_incomingBuffer.size())
        m_incomingBuffer += QByteArray(packetOffset + packetLength - m_incomingBuffer.size(), 0);
    QDataStream output(&m_incomingBuffer, QIODevice::WriteOnly);
    output.device()->seek(packetOffset);
    output.setByteOrder(QDataStream::LittleEndian);
    m_codec->decode(stream, output);

    // check whether we are running late
    if (m_incomingBuffer.size() > m_incomingMaximum)
    {
        const qint64 droppedSize = m_incomingBuffer.size() - m_incomingMinimum;
        emit logMessage(QXmppLogger::DebugMessage,
            QString("RTP buffer is too full, dropping %1 bytes")
                .arg(QString::number(droppedSize)));
        m_incomingBuffer = m_incomingBuffer.right(m_incomingMinimum);
        m_incomingStamp += droppedSize / SAMPLE_BYTES;
    }
    // check whether we have filled the initial buffer
    if (m_incomingBuffer.size() >= m_incomingMinimum)
        m_incomingBuffering = false;
    if (!m_incomingBuffering)
        emit readyRead();
}

qint64 RtpChannel::readData(char * data, qint64 maxSize)
{
    // if we are filling the buffer, return empty samples
    if (m_incomingBuffering)
    {
        memset(data, 0, maxSize);
        return maxSize;
    }

    qint64 readSize = qMin(maxSize, qint64(m_incomingBuffer.size()));
    memcpy(data, m_incomingBuffer.constData(), readSize);
    m_incomingBuffer.remove(0, readSize);
    if (readSize < maxSize)
    {
        emit logMessage(QXmppLogger::InformationMessage,
            QString("QXmppCall::readData missing %1 bytes").arg(QString::number(maxSize - readSize)));
        memset(data + readSize, 0, maxSize - readSize);
    }
    m_incomingStamp += readSize / SAMPLE_BYTES;
    return maxSize;
}

qint64 RtpChannel::writeData(const char * data, qint64 maxSize)
{
    if (!m_codec)
    {
        emit logMessage(QXmppLogger::WarningMessage,
            QLatin1String("QXmppCall::writeData before codec was set"));
        return -1;
    }

    m_outgoingBuffer += QByteArray::fromRawData(data, maxSize);
    while (m_outgoingBuffer.size() >= m_outgoingChunk)
    {
        QByteArray header;
        QDataStream stream(&header, QIODevice::WriteOnly);
        quint8 version = RTP_VERSION << 6;
        stream << version;
        quint8 type = m_payloadId;
        if (m_outgoingMarker)
        {
            type |= 0x80;
            m_outgoingMarker= false;
        }
        stream << type;
        stream << ++m_outgoingSequence;
        stream << m_outgoingStamp;
        const quint32 ssrc = 0;
        stream << ssrc;

        QByteArray chunk = m_outgoingBuffer.left(m_outgoingChunk);
        QDataStream input(chunk);
        input.setByteOrder(QDataStream::LittleEndian);
        m_outgoingStamp += m_codec->encode(input, stream);

        // FIXME: write data
#if 0
        if (m_connection->writeDatagram(RTP_COMPONENT, header) < 0)
            emit logMessage(QXmppLogger::WarningMessage,
                QLatin1String("QXmppCall:writeData could not send audio data"));
#endif
#ifdef QXMPP_DEBUG_RTP
        else
            emit logMessage(QXmppLogger::SentMessage,
                QString("RTP packet seq %1 stamp %2 size %3")
                    .arg(QString::number(m_outgoingSequence))
                    .arg(QString::number(m_outgoingStamp))
                    .arg(QString::number(header.size() - 12)));
#endif

        m_outgoingBuffer.remove(0, chunk.size());
    }

    m_writtenSinceLastEmit += maxSize;
    if (!m_signalsEmitted && !signalsBlocked()) {
        m_signalsEmitted = true;
        QMetaObject::invokeMethod(this, "emitSignals", Qt::QueuedConnection);
    }

    return maxSize;
}
