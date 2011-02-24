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

TurnTester::TurnTester(QXmppTurnAllocation *allocation)
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
    qDebug() << "DATA" << data;
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
    QXmppTurnAllocation allocation;
    allocation.setServer(host);
    allocation.setUsername(QLatin1String("test"));
    allocation.setPassword(QLatin1String("test"));
    QObject::connect(&allocation, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        &logger, SLOT(log(QXmppLogger::MessageType,QString)));

    TurnTester turnTester(&allocation);
    QObject::connect(&turnTester, SIGNAL(finished()),
                     &app, SLOT(quit()));

    allocation.connectToHost();
#endif
    return app.exec();
}
