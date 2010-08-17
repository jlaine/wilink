#include <cstdlib>

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppStun.h"

#include "stun.h"

enum MessageType {
    BindingRequest       = 0x0001,
};

// http://tools.ietf.org/html/rfc5780
enum StunTest
{
    PrimaryAddressAndPort,   // TEST I
    AlternateAddress,        // TEST II
    AlternateAddressAndPort, // TEST III 
};

StunTester::StunTester()
    : m_test(0)
{
    m_socket = new QUdpSocket;
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    Q_ASSERT(m_socket->bind());
}

void StunTester::readyRead()
{
    const qint64 size = m_socket->pendingDatagramSize();
    QHostAddress remoteHost;
    quint16 remotePort;
    QByteArray buffer(size, 0);
    m_socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    QXmppStunMessage msg;
    QStringList errors;
    if (msg.decode(buffer, QString(), &errors))
        qDebug() << "Received response from" << remoteHost << msg.toString();
    else
        qDebug() << "Bad response from" << remoteHost << errors;

    // Test I response
    if (m_test == PrimaryAddressAndPort)
    {
        qDebug() << "Have UDP connectivity";
        m_alternateHost = msg.otherHost;
        m_alternatePort = msg.otherPort;

        // Test II request
        m_test = AlternateAddress;
        QXmppStunMessage req;
        req.setType(BindingRequest);
        sendPacket(req, m_alternateHost, m_primaryPort);
    }
}

void StunTester::run(const QHostAddress &host, quint16 port)
{
    m_primaryHost = host;
    m_primaryPort = port;

    // Test I request
    m_test = PrimaryAddressAndPort;
    QXmppStunMessage msg;
    msg.setType(BindingRequest);
    sendPacket(msg, m_primaryHost, m_primaryPort);
}

bool StunTester::sendPacket(const QXmppStunMessage &req, const QHostAddress &host, quint16 port)
{
    qDebug() << "Sending request to" << host << port << req.toString();
    if (m_socket->writeDatagram(req.encode(), host, port) < 0)
    {
        qWarning("Could not send packet");
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // parse command line arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: stun <hostname>\n");
        return EXIT_FAILURE;
    }
    QHostInfo hostInfo = QHostInfo::fromName(argv[1]);
    if (hostInfo.addresses().isEmpty())
    {
        qWarning("Could not lookup host");
        return EXIT_FAILURE;
    }

    StunTester tester;
    tester.run(hostInfo.addresses().first(), 3478);

    return app.exec();
}
