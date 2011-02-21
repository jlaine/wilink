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

#ifndef __WILINK_PHONE_STUN_H__
#define __WILINK_PHONE_STUN_H__

#include <QHostAddress>

#include "QXmppLogger.h"
#include "QXmppStun.h"

class QUdpSocket;
class QTimer;

class StunTester : public QXmppLoggable
{
    Q_OBJECT

public:
    enum ConnectionType
    {
        NoConnection = 0,
        DirectConnection,
        NattedConnection,
    };

    StunTester(QObject *parent = 0);
    bool bind(const QHostAddress &address);
    void setServer(const QHostAddress &server, quint16 port);

signals:
    void finished(StunTester::ConnectionType result);

public slots:
    void start();

private slots:
    void readyRead();
    void timeout();

private:
    void sendRequest();

    QXmppStunMessage request;
    QHostAddress requestAddress;
    quint16 requestPort;
    int retries;
    QHostAddress serverAddress;
    quint16 serverPort;
    QUdpSocket *socket;
    int step;
    QTimer *timer;
};

#endif
