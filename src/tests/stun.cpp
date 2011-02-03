#include <cstdlib>

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>

#include "QXmppLogger.h"
#include "QXmppStun.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // parse command line arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: stun <hostname>\n");
        return EXIT_FAILURE;
    }

    QXmppLogger logger;
    logger.setLoggingType(QXmppLogger::StdoutLogging);

    // lookup STUN server
    const QString hostName = QString::fromLocal8Bit(argv[1]);
    QHostAddress host;
    QHostInfo hostInfo = QHostInfo::fromName(hostName);
    foreach (const QHostAddress &address, hostInfo.addresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            host = address;
            break;
        }
    }
    if (host.isNull())
    {
        fprintf(stderr, "Could not lookup STUN server %s", argv[1]);
        return EXIT_FAILURE;
    }

    QXmppIceConnection connection(true);
    connection.setStunServer(host);
    QObject::connect(&connection, SIGNAL(localCandidatesChanged()),
        &app, SLOT(quit()));
    QObject::connect(&connection, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));
    connection.addComponent(1);
    connection.addComponent(2);

    connection.bind(QXmppIceComponent::discoverAddresses());
    connection.connectToHost();
    return app.exec();
}
