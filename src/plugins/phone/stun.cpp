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

#include <QDateTime>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QPair>
#include <QUdpSocket>
#include <QTimer>

#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "stun.h"

#define QXMPP_DEBUG_STUN

#define STUN_RETRY_MS   500

enum StunStep {
    StunConnectivity = 0,
    StunChangeServer,
};

StunTester::StunTester(QObject *parent)
    : QXmppLoggable(parent),
    retries(0),
    serverPort(0),
    step(0)
{
    socket = new QUdpSocket(this);
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(readyRead()));

    timer = new QTimer(this);
    timer->setInterval(STUN_RETRY_MS);
    connect(timer, SIGNAL(timeout()),
            this, SLOT(timeout()));
}

bool StunTester::bind(const QHostAddress &address)
{
    if (!socket->bind(address, 0)) {
        warning(QString("Could not start listening for STUN on %1").arg(
            address.toString()));
        return false;
    }
    debug(QString("Listening for STUN on %1:%2").arg(
        socket->localAddress().toString(),
        QString::number(socket->localPort())));
    return true;
}

void StunTester::readyRead()
{
    if (!socket->hasPendingDatagrams())
        return;

    // receive datagram
    const qint64 size = socket->pendingDatagramSize();
    QByteArray buffer(size, 0);
    QHostAddress remoteHost;
    quint16 remotePort;
    socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    // decode STUN
    QXmppStunMessage response;
    if (!response.decode(buffer) || response.id() != request.id())
        return;

#ifdef QXMPP_DEBUG_STUN
    logReceived(QString("STUN packet from %1 port %2\n%3").arg(
        remoteHost.toString(),
        QString::number(remotePort),
        response.toString()));
#endif

    // examine result
    timer->stop();
    if (step == 0) {
        if ((response.xorMappedHost == socket->localAddress() && response.xorMappedPort == socket->localPort()) ||
                (response.mappedHost == socket->localAddress() && response.mappedPort == socket->localPort())) {
            emit finished(DirectConnection);
            return;
        }
        if (response.changedHost.isNull() || !response.changedPort) {
            emit finished(NattedConnection);
            return;
        }

        step++;

        // send next request
        request = QXmppStunMessage();
        request.setCookie(qrand());
        request.setId(generateRandomBytes(12));
        request.setType(QXmppStunMessage::Binding | QXmppStunMessage::Request);

        requestAddress = response.changedHost;
        requestPort = response.changedPort;

        sendRequest();
    } else {
        emit finished(NattedConnection);
    }
}

void StunTester::sendRequest()
{
#ifdef QXMPP_DEBUG_STUN
    logSent(QString("STUN packet to %1 port %2\n%3").arg(requestAddress.toString(),
            QString::number(requestPort), request.toString()));
#endif
    socket->writeDatagram(request.encode(QString(), 0), requestAddress, requestPort);
    timer->start();
}

void StunTester::setServer(const QHostAddress &server, quint16 port)
{
    serverAddress = server;
    serverPort = port;
}

void StunTester::start()
{
    if (serverAddress.isNull() || !serverPort) {
        warning("STUN tester is missing server address or port");
        emit finished(NoConnection);
        return;
    }
    retries = 0;
    step = 0;

    // initial binding request
    request = QXmppStunMessage();
    request.setCookie(qrand());
    request.setId(generateRandomBytes(12));
    request.setType(QXmppStunMessage::Binding | QXmppStunMessage::Request);

    requestAddress = serverAddress;
    requestPort = serverPort;

    sendRequest();
}

void StunTester::timeout()
{
    timer->stop();
    if (retries <= 4) {
        retries++;
        sendRequest();
        return;
    }

    warning("STUN timeout");
    if (step == 0)
        emit finished(NoConnection);
    else
        emit finished(NattedConnection);
}
