#include <QIODevice>

#include "QXmppLogger.h"

class QXmppCodec;

class RtpChannel : public QIODevice
{
    Q_OBJECT

public:
    RtpChannel(QObject *parent = 0);

signals:
    /// This signal is emitted to send logging messages.
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

protected:
    /// \cond
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);
    /// \endcond

private:
    // signals
    bool m_signalsEmitted;
    qint64 m_writtenSinceLastEmit;

    // RTP
    QXmppCodec *m_codec;
    quint8 m_payloadId;

    QByteArray m_incomingBuffer;
    bool m_incomingBuffering;
    int m_incomingMinimum;
    int m_incomingMaximum;
    quint16 m_incomingSequence;
    quint32 m_incomingStamp;

    quint16 m_outgoingChunk;
    QByteArray m_outgoingBuffer;
    bool m_outgoingMarker;
    quint16 m_outgoingSequence;
    quint32 m_outgoingStamp;

};

