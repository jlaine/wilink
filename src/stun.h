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

private slots:
    void readyRead();

private:
    bool sendPacket(const QXmppStunMessage &req, const QHostAddress &host, quint16 port);

    QXmppIceConnection *m_connection;
    QUdpSocket *m_socket;
};

