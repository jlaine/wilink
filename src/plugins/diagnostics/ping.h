/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#ifndef __DIAGNOSTICS_PING_H__
#define __DIAGNOSTICS_PING_H__

#include <QHostAddress>
#include <QList>
#include <QXmlStreamWriter>

class QDomElement;

/** The Ping class represents an ICMP ping result.
 */
class Ping
{
public:
    Ping();

    QHostAddress hostAddress;

    // in milliseconds
    float minimumTime;
    float maximumTime;
    float averageTime;

    int sentPackets;
    int receivedPackets;

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
};

/** The Traceroute class represents a traceroute result.
 */
class Traceroute : public QList<Ping>
{
public:
    QHostAddress hostAddress() const { return m_hostAddress; }
    void setHostAddress(const QHostAddress &hostAddress) { m_hostAddress = hostAddress; };

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

private:
    QHostAddress m_hostAddress;
};

class NetworkInfo
{
public:
    static Ping ping(const QHostAddress &host, int maxPackets = 1);
    static Traceroute traceroute(const QHostAddress &host, int maxPackets = 1, int maxHops = 0);
};

#endif
