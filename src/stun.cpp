#include <cstdlib>

#include <QCoreApplication>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppStun.h"

#include "stun.h"

StunTester::StunTester()
{
    m_socket = new QUdpSocket;
    Q_ASSERT(m_socket->bind());

    QHostInfo info = QHostInfo::fromName("stun.ekiga.net");
    Q_ASSERT(!info.addresses().isEmpty());
    
    m_host = info.addresses().first();
    m_port = 3478;

    QXmppStunMessage msg;
    msg.setType(0x0001);
    if (m_socket->writeDatagram(msg.encode(), m_host, m_port) < 0)
        qWarning("Could not send packet");
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    StunTester tester;
    return app.exec();
}
