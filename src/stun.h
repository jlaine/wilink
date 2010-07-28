#include <QObject>
#include <QHostAddress>

class QUdpSocket;

class StunTester : public QObject
{
    Q_OBJECT

public:
    StunTester();

private:
    QHostAddress m_host;
    quint16 m_port;
    QUdpSocket *m_socket;
};

