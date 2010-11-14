#include <QIODevice>

#include "QXmppLogger.h"

class RtpChannelPrivate;
class QXmppCodec;

class RtpChannel : public QIODevice
{
    Q_OBJECT

public:
    RtpChannel(QObject *parent = 0);
    ~RtpChannel();

signals:
    /// This signal is emitted to send logging messages.
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void datagramReceived(const QByteArray &buffer);

protected:
    /// \cond
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);
    /// \endcond

private:
    RtpChannelPrivate * const d;
};

