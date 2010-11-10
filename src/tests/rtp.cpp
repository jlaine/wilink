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
