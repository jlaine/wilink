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

StunTester::StunTester()
{
    m_connection = new QXmppIceConnection(true, this);

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

    qDebug() << "Have UDP connectivity";
}

void StunTester::run(const QHostAddress &host, quint16 port)
{
    // Test I request
    QXmppStunMessage msg;
    msg.setType(BindingRequest);
    sendPacket(msg, host, port);
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

    // lookup STUN server
    QHostAddress stunAddress;
    const quint16 stunPort = 3478;
    QHostInfo hostInfo = QHostInfo::fromName(argv[1]);
    foreach (const QHostAddress &address, hostInfo.addresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            stunAddress = address;
            break;
        }
    }
    if (hostInfo.addresses().isEmpty())
    {
        qWarning("Could not lookup host");
        return EXIT_FAILURE;
    }

    StunTester tester;
    tester.run(stunAddress, stunPort);

    return app.exec();
}
