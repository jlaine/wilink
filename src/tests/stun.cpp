/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>

#include <QCoreApplication>
#include <QHostInfo>
#include <QTimer>

#include "QXmppLogger.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "stun.h"

TurnTester::TurnTester(QXmppTurnAllocation *allocation, const QHostAddress &host, quint16 port)
    : m_allocation(allocation),
    m_host(host),
    m_port(port)
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
    m_allocation->writeDatagram(QByteArray("12345\n"), m_host, m_port);
}

void TurnTester::datagramReceived(const QByteArray &data, const QHostAddress &host, quint16 port)
{
    qDebug() << "DATA" << data;
    m_allocation->disconnectFromHost();
}

static QHostAddress lookup(const QString &hostName)
{
    QHostInfo hostInfo = QHostInfo::fromName(hostName);
    foreach (const QHostAddress &address, hostInfo.addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
            return address;
    }
    return QHostAddress();
}

static void usage()
{
    fprintf(stderr, "Usage: stun <hostname> <peer_host>\n");
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // parse command line arguments
    if (argc < 3)
    {
        usage();
        return EXIT_FAILURE;
    }

    QHostAddress stunHost;
    QHostAddress turnHost;
    const QString turnUsername = QLatin1String("test");
    const QString turnPassword = QLatin1String("test");
    for (int i = 1; i < argc - 2; ++i) {
        if (!strcmp(argv[i], "-s")) {
            // lookup STUN server
            const QString hostName = QString::fromLocal8Bit(argv[++i]);
            stunHost = lookup(hostName);
            if (stunHost.isNull()) {
                qWarning("Could not lookup STUN server %s", qPrintable(hostName));
                return EXIT_FAILURE;
            }
        } else if (!strcmp(argv[i], "-t")) {
            // lookup TURN server
            const QString hostName = QString::fromLocal8Bit(argv[++i]);
            turnHost = lookup(hostName);
            if (turnHost.isNull()) {
                qWarning("Could not lookup TURN server %s", qPrintable(hostName));
                return EXIT_FAILURE;
            }
        }
    }

    // lookup peer
    const QString peerName = QString::fromLocal8Bit(argv[argc - 1]);
    const quint16 peerPort = 40000;
    const QHostAddress peerHost = lookup(peerName);
    if (peerHost.isNull()) {
        qWarning("Could not lookup peer %s", qPrintable(peerName));
        return EXIT_FAILURE;
    }

    QXmppLogger logger;
    logger.setLoggingType(QXmppLogger::StdoutLogging);

    QXmppIceConnection connection(true);
    connection.setStunServer(stunHost);
    connection.setTurnServer(turnHost);
    connection.setTurnUsername(turnUsername);
    connection.setTurnPassword(turnPassword);
    QObject::connect(&connection, SIGNAL(connected()),
        &app, SLOT(quit()));
    QObject::connect(&connection, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));
    connection.addComponent(1);
    connection.bind(QXmppIceComponent::discoverAddresses());

    QXmppJingleCandidate candidate;
    candidate.setComponent(1);
    candidate.setHost(peerHost);
    candidate.setPort(peerPort);
    candidate.setProtocol("udp");
    candidate.setType(QXmppJingleCandidate::HostType);

    connection.addRemoteCandidate(candidate);
    connection.setRemoteUser("test");
    connection.setRemotePassword("test");

    connection.connectToHost();
    return app.exec();
}
