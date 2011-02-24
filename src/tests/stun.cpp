#include <cstdlib>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QUdpSocket>
#include <QTimer>

#include "QXmppLogger.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "stun.h"

static bool operator<(const QHostAddress &a1, const QHostAddress &a2)
{
    return a1.toString() < a2.toString();
}

TurnAllocation::TurnAllocation(QObject *parent)
    : QXmppLoggable(parent),
    m_relayedPort(0),
    m_turnPort(0),
    m_lifetime(600),
    m_state(UnconnectedState)
{
    socket = new QUdpSocket(this);
    socket->bind();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
}

void TurnAllocation::connectToHost()
{
    if (m_state != UnconnectedState)
        return;

    // send allocate request
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Allocate);
    request.setId(generateRandomBytes(12));
    request.setLifetime(m_lifetime);
    request.setRequestedTransport(0x11);
    writeStun(request);

    // update state
    setState(ConnectingState);
}

void TurnAllocation::disconnectFromHost()
{
    timer->stop();
    if (m_state != ConnectedState)
        return;

    // send refresh request with zero lifetime
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Refresh);
    request.setId(generateRandomBytes(12));
    request.setNonce(m_nonce);
    request.setRealm(m_realm);
    request.setUsername(m_username);
    request.setLifetime(0);
    writeStun(request);

    // update state
    setState(ClosingState);
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

    // handle authentication
    if (message.messageClass() == QXmppStunMessage::Error &&
        message.errorCode == 401 &&
        message.id() == m_request.id())
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
            setState(UnconnectedState);
            return;
        }
        if (message.xorRelayedHost.isNull() ||
            message.xorRelayedHost.protocol() != QAbstractSocket::IPv4Protocol ||
            !message.xorRelayedPort) {
            warning("Allocation did not yield a valid relayed address");
            setState(UnconnectedState);
            return;
        }

        // store relayed address
        m_relayedHost = message.xorRelayedHost;
        m_relayedPort = message.xorRelayedPort;

        // schedule refresh
        m_lifetime = message.lifetime();
        timer->start((m_lifetime - 60) * 1000);

        setState(ConnectedState);

    } else if (message.messageMethod() == QXmppStunMessage::CreatePermission) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            warning(QString("CreatePermission failed: %1 %2").arg(
                QString::number(message.errorCode), message.errorPhrase));
            return;
        }

        // store permission
        //m_permissions << qMakePair(message.xorPeerHost, message.xorPeerPort);

    } else if (message.messageMethod() == QXmppStunMessage::Data &&
               message.messageClass() == QXmppStunMessage::Indication) {

        const Address addr = qMakePair(message.xorPeerHost, message.xorPeerPort);
        if (m_permissions.contains(addr)) {
            emit datagramReceived(message.data(),
                message.xorPeerHost, message.xorPeerPort);
        }

    } else if (message.messageMethod() == QXmppStunMessage::Refresh) {
        if (message.messageClass() == QXmppStunMessage::Error) {
            warning(QString("Refresh failed: %1 %2").arg(
                QString::number(message.errorCode), message.errorPhrase));
            setState(UnconnectedState);
            return;
        }

        if (m_state == ClosingState) {
            setState(UnconnectedState);
            return;
        }

        // schedule refresh
        m_lifetime = message.lifetime();
        timer->start((m_lifetime - 60) * 1000);
    }
}

void TurnAllocation::refresh()
{
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Refresh);
    request.setId(generateRandomBytes(12));
    request.setNonce(m_nonce);
    request.setRealm(m_realm);
    request.setUsername(m_username);
    writeStun(request);
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

void TurnAllocation::setState(AllocationState state)
{
    if (state == m_state)
        return;
    m_state = state;
    if (m_state == ConnectedState) {
        emit connected();
    } else if (m_state == UnconnectedState) {
        timer->stop();
        emit disconnected();
    }
}

void TurnAllocation::writeDatagram(const QByteArray &data, const QHostAddress &host, quint16 port)
{
    const Address addr = qMakePair(host, port);

    if (!m_permissions.contains(addr)) {
        // create permission
        QXmppStunMessage request;
        request.setType(QXmppStunMessage::CreatePermission);
        request.setId(generateRandomBytes(12));
        request.setNonce(m_nonce);
        request.setRealm(m_realm);
        request.setUsername(m_username);
        request.xorPeerHost = host;
        request.xorPeerPort = port;
        writeStun(request);

        m_permissions.insert(addr, request.id());
    }

    // send data
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Send | QXmppStunMessage::Indication);
    request.setId(generateRandomBytes(12));
    request.setNonce(m_nonce);
    request.setRealm(m_realm);
    request.setUsername(m_username);
    request.xorPeerHost = host;
    request.xorPeerPort = port;
    request.setData(data);
    writeStun(request);
}

qint64 TurnAllocation::writeStun(const QXmppStunMessage &message)
{
    qint64 ret = socket->writeDatagram(message.encode(m_key),
        m_turnHost, m_turnPort);
    if (message.messageClass() == QXmppStunMessage::Request)
        m_request = message;
    logSent(QString("STUN packet to %1 port %2\n%3").arg(
            m_turnHost.toString(),
            QString::number(m_turnPort),
            message.toString()));
    return ret;
}

TurnTester::TurnTester(TurnAllocation *allocation)
    : m_allocation(allocation)
{
    connect(m_allocation, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(m_allocation, SIGNAL(datagramReceived(QByteArray,QHostAddress,quint16)),
            this, SLOT(datagramReceived(QByteArray,QHostAddress,quint16)));
    connect(m_allocation, SIGNAL(disconnected()),
            this, SIGNAL(finished()));
}

void TurnTester::connected()
{
    m_allocation->writeDatagram(QByteArray("12345\n"), QHostAddress("1.2.3.4"), 12345);
}

void TurnTester::datagramReceived(const QByteArray &data, const QHostAddress &host, quint16 port)
{
    emit finished();
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

    TurnTester turnTester(&turn);
    QObject::connect(&turnTester, SIGNAL(finished()),
                     &app, SLOT(quit()));

    turn.connectToHost();
#endif
    return app.exec();
}
