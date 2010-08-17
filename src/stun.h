#include <QHostAddress>
#include <QObject>

class QUdpSocket;

class QXmppIceConnection;
class QXmppStunMessage;

class StunTester : public QObject
{
    Q_OBJECT

public:
    StunTester();
    void run(const QHostAddress &host, quint16 port);

signals:
    void finished();

private:
    QXmppIceConnection *m_connection;
};

