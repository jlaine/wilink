#include <cstdlib>

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppStun.h"

#include "stun.h"

StunTester::StunTester()
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
}

void StunTester::run(const QHostAddress &host, quint16 port)
{
    QXmppStunMessage msg;
    msg.setType(0x0001);
    qDebug() << "Sending request to" << host << msg.toString();
    if (m_socket->writeDatagram(msg.encode(), host, port) < 0)
        qWarning("Could not send packet");
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
