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

TurnAllocation::TurnAllocation(QObject *parent)
    : QXmppLoggable(parent),
    m_relayedPort(0),
    m_turnPort(0),
    m_state(UnconnectedState)
{
    socket = new QUdpSocket(this);
    socket->bind();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    m_peerHost = QHostAddress("1.2.3.4");
    m_peerPort = 1234;
}

void TurnAllocation::connectToHost()
{
    if (m_state != UnconnectedState)
        return;

    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Allocate);
    request.setId(generateRandomBytes(12));
    request.setLifetime(165);
    request.setRequestedTransport(0x11);
    writeStun(request);
    m_state = ConnectingState;
}

void TurnAllocation::disconnectFromHost()
{
    if (m_state != ConnectedState)
        return;

    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Refresh);
    request.setId(generateRandomBytes(12));
    request.setNonce(m_nonce);
    request.setRealm(m_realm);
    request.setUsername(m_username);
    request.setLifetime(0);
    writeStun(request);
    m_state = ClosingState;
}

void TurnAllocation::readyRead()
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
            warning(error);
        return;
    }

    logReceived(QString("STUN packet from %1 port %2\n%3").arg(
            remoteHost.toString(),
            QString::number(remotePort),
            message.toString()));

    if (message.id() != m_request.id()) {
        warning("Bad STUN packet ID");
        return;
    }

    // handle authentication
    if (message.messageClass() == QXmppStunMessage::Error &&
        message.errorCode == 401)
    {
        if (m_nonce != message.nonce() ||
            m_realm != message.realm()) {
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

    if (message.messageMethod() == QXmppStunMessage::Allocate) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            warning(QString("Allocation failed: %1 %2").arg(
                QString::number(message.errorCode), message.errorPhrase));
            m_state = UnconnectedState;
            emit disconnected();
            return;
        }
        if (message.xorRelayedHost.isNull() ||
            message.xorRelayedHost.protocol() != QAbstractSocket::IPv4Protocol ||
            !message.xorRelayedPort) {
            warning("Allocation did not yield a valid relayed address");
            m_state = UnconnectedState;
            emit disconnected();
            return;
        }

        // store relayed address
        m_relayedHost = message.xorRelayedHost;
        m_relayedPort = message.xorRelayedPort;
        m_state = ConnectedState;
        emit connected();

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

    } else if (message.messageMethod() == QXmppStunMessage::CreatePermission) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            warning(QString("CreatePermission failed: %1 %2").arg(
                QString::number(message.errorCode), message.errorPhrase));
            m_state = UnconnectedState;
            emit disconnected();
            return;
        }

        disconnectFromHost();
        return;

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

    } else if (message.messageMethod() == QXmppStunMessage::Refresh) {
        m_state = UnconnectedState;
        emit disconnected();
    }
}

QHostAddress TurnAllocation::relayedHost() const
{
    return m_relayedHost;
}

quint16 TurnAllocation::relayedPort() const
{
    return m_relayedPort;
}

void TurnAllocation::setServer(const QHostAddress &host, quint16 port)
{
    m_turnHost = host;
    m_turnPort = port;
}

void TurnAllocation::setPassword(const QString &password)
{
    m_password = password;
}

void TurnAllocation::setUsername(const QString &username)
{
    m_username = username;
}

qint64 TurnAllocation::writeStun(const QXmppStunMessage &message)
{
    qint64 ret = socket->writeDatagram(message.encode(m_key),
        m_turnHost, m_turnPort);
    m_request = message;
    logSent(QString("STUN packet to %1 port %2\n%3").arg(
            m_turnHost.toString(),
            QString::number(m_turnPort),
            message.toString()));
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
    TurnAllocation turn;
    turn.setServer(host);
    turn.setUsername(QLatin1String("test"));
    turn.setPassword(QLatin1String("test"));
    QObject::connect(&turn, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));
    QObject::connect(&turn, SIGNAL(disconnected()),
                     &app, SLOT(quit()));
    turn.connectToHost();
#endif
    return app.exec();
}
