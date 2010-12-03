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

    QXmppIceConnection connection(true);
    connection.setStunServer(QString::fromLocal8Bit(argv[1]));
    QObject::connect(&connection, SIGNAL(localCandidatesChanged()),
        &app, SLOT(quit()));
    QObject::connect(&connection, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));
    connection.addComponent(1);
    connection.addComponent(2);

    connection.bind(QXmppStunSocket::discoverAddresses());
    connection.connectToHost();
    return app.exec();
}
