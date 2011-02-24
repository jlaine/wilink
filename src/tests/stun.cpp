#include <cstdlib>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppLogger.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "stun.h"

Turn::Turn(QObject *parent)
    : QObject(parent),
    m_turnPort(0)
{
    socket = new QUdpSocket(this);
    socket->bind();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    m_peerHost = QHostAddress("1.2.3.4");
    m_peerPort = 1234;
}

void Turn::connectToHost()
{
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Allocate);
    request.setId(generateRandomBytes(12));
    request.setLifetime(165);
    request.setRequestedTransport(0x11);
    writeStun(request);
}

void Turn::readyRead()
{
    const qint64 size = socket->pendingDatagramSize();
    QHostAddress remoteHost;
    quint16 remotePort;

    QByteArray buffer(size, 0);
    socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    // parse STUN message
    QXmppStunMessage message;
    QStringList errors;
    if (!message.decode(buffer, QByteArray(), &errors)) {
        foreach (const QString &error, errors)
            qWarning() << error;
        return;
    }

    qDebug() << QString("STUN packet from %1 port %2\n%3").arg(remoteHost.toString(),
            QString::number(remotePort),
            message.toString());

    if (message.id() != m_request.id()) {
        qWarning("Bad STUN packet ID");
        return;
    }

    // handle authentication
    if (message.messageClass() == QXmppStunMessage::Error &&
        message.errorCode == 401)
    {
        if (m_nonce != message.nonce() ||
            m_realm == message.realm()) {
            // update long-term credentials
            m_nonce = message.nonce();
            m_realm = message.realm();
            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData((m_username + ":" + m_realm + ":" + m_password).toUtf8());
            m_key = hash.result();

            // retry request
            QXmppStunMessage request(m_request);
            request.setId(generateRandomBytes(12));
            request.setNonce(m_nonce);
            request.setRealm(m_realm);
            request.setUsername(m_username);
            writeStun(request);
            return;
        }
    }

    if (m_request.messageMethod() == QXmppStunMessage::Allocate) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            qWarning("Allocation failed, code %i", message.errorCode);
            emit finished();
            return;
        }

        // create permission
        QXmppStunMessage request;
        request.setType(QXmppStunMessage::CreatePermission);
        request.setId(generateRandomBytes(12));
        request.setNonce(m_nonce);
        request.setRealm(m_realm);
        request.setUsername(m_username);
        request.xorPeerHost = m_peerHost;
        request.xorPeerPort = m_peerPort;
        writeStun(request);

    } else if (m_request.messageMethod() == QXmppStunMessage::CreatePermission) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            qWarning("CreatePermission failed, code %i", message.errorCode);
            emit finished();
            return;
        }

        // send some data
        QXmppStunMessage request;
        request.setType(QXmppStunMessage::Send | QXmppStunMessage::Indication);
        request.setId(generateRandomBytes(12));
        request.setNonce(m_nonce);
        request.setRealm(m_realm);
        request.setUsername(m_username);
        request.xorPeerHost = m_peerHost;
        request.xorPeerPort = m_peerPort;
        request.setData(QByteArray("12345\n"));
        writeStun(request);
    }
}

void Turn::setTurnServer(const QHostAddress &host, quint16 port)
{
    m_turnHost = host;
    m_turnPort = port;
}

void Turn::setPassword(const QString &password)
{
    m_password = password;
}

void Turn::setUsername(const QString &username)
{
    m_username = username;
}

qint64 Turn::writeStun(const QXmppStunMessage &message)
{
    qint64 ret = socket->writeDatagram(message.encode(m_key),
        m_turnHost, m_turnPort);
    m_request = message;
    qDebug() << QString("STUN packet to %1 port %2\n%3").arg(
            m_turnHost.toString(),
            QString::number(m_turnPort),
            message.toString());
    return ret;
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

#if 0
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
#else
    Turn turn;
    turn.setTurnServer(host);
    turn.setUsername(QLatin1String("test"));
    turn.setPassword(QLatin1String("test"));
    turn.connectToHost();
    QObject::connect(&turn, SIGNAL(finished()),
                     &app, SLOT(quit()));
#endif
    return app.exec();
}
