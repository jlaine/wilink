#include <QIODevice>

#include "QXmppLogger.h"

class QXmppCodec;
class QXmppJinglePayloadType;
class RtpChannelPrivate;

class RtpChannel : public QIODevice
{
    Q_OBJECT

public:
    enum CodecId {
        G711u = 0,
        GSM = 3,
        G723 = 4,
        G711a = 8,
        G722 = 9,
        L16Stereo = 10,
        L16Mono = 11,
        G728 = 15,
        G729 = 18,
    };

    RtpChannel(QObject *parent = 0);
    ~RtpChannel();

    QXmppJinglePayloadType payloadType() const;
    void setPayloadType(const QXmppJinglePayloadType &payloadType);

    void setSocket(QIODevice *socket);

    /// \cond
    qint64 bytesAvailable() const;
    bool isSequential() const;
    /// \endcond

signals:
    /// This signal is emitted to send logging messages.
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void datagramReceived(const QByteArray &buffer);
    void readFromSocket();

protected:
    /// \cond
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);
    /// \endcond

private:
    RtpChannelPrivate * const d;
};

