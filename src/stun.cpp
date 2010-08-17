#include <cstdlib>

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppLogger.h"
#include "QXmppStun.h"

#include "stun.h"

enum MessageType {
    BindingRequest       = 0x0001,
};

StunTester::StunTester()
{
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

    QXmppLogger logger;
    logger.setLoggingType(QXmppLogger::StdoutLogging);

    QXmppIceConnection connection(true);
    connection.setStunServer(stunAddress, stunPort);
    QObject::connect(&connection, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));
    connection.addComponent(1);
    return app.exec();
}
