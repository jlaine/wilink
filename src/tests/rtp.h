#include <QIODevice>

#include "QXmppLogger.h"

class QXmppCodec;
class RtpChannelPrivate;

class RtpChannel : public QIODevice
{
    Q_OBJECT

public:
    RtpChannel(QObject *parent = 0);
    ~RtpChannel();

    void setCodec(QXmppCodec *codec);
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

