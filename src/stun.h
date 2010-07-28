#include <QObject>

class QHostAddress;
class QUdpSocket;

class StunTester : public QObject
{
    Q_OBJECT

public:
    StunTester();
    void run(const QHostAddress &host, quint16 port);

private:
    QUdpSocket *m_socket;
};

