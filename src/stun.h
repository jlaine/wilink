#include <QHostAddress>
#include <QObject>

class QUdpSocket;

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

    QUdpSocket *m_socket;
    int m_test;
    QHostAddress m_primaryHost;
    quint16 m_primaryPort;
    QHostAddress m_alternateHost;
    quint16 m_alternatePort;
};

