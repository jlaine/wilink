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

#include <QDomElement>
#include <QProcess>

#include "ping.h"

Ping::Ping()
    : m_minimumTime(0.0),
    m_maximumTime(0.0),
    m_averageTime(0.0),
    m_sentPackets(0),
    m_receivedPackets(0)
{
}

QHostAddress Ping::hostAddress() const
{
    return m_hostAddress;
}

void Ping::setHostAddress(const QHostAddress &hostAddress)
{
    m_hostAddress = hostAddress;
}

float Ping::minimumTime() const
{
    return m_minimumTime;
}

void Ping::setMinimumTime(float minimumTime)
{
    m_minimumTime = minimumTime;
}

float Ping::maximumTime() const
{
    return m_maximumTime;
}

void Ping::setMaximumTime(float maximumTime)
{
    m_maximumTime = maximumTime;
}

float Ping::averageTime() const
{
    return m_averageTime;
}

void Ping::setAverageTime(float averageTime)
{
    m_averageTime = averageTime;
}

int Ping::sentPackets() const
{
    return m_sentPackets;
}

void Ping::setSentPackets(int sentPackets)
{
    m_sentPackets = sentPackets;
}

int Ping::receivedPackets() const
{
    return m_receivedPackets;
}

void Ping::setReceivedPackets(int receivedPackets)
{
    m_receivedPackets = receivedPackets;
}

void Ping::parse(const QDomElement &element)
{
    m_hostAddress = QHostAddress(element.attribute("hostAddress"));

    m_minimumTime = element.attribute("minimumTime").toFloat();
    m_maximumTime = element.attribute("maximumTime").toFloat();
    m_averageTime = element.attribute("averageTime").toFloat();

    m_sentPackets = element.attribute("sentPackets").toInt();
    m_receivedPackets = element.attribute("receivedPackets").toInt();
}

void Ping::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("ping");
    writer->writeAttribute("hostAddress", m_hostAddress.toString());

    writer->writeAttribute("minimumTime", QString::number(m_minimumTime));
    writer->writeAttribute("maximumTime", QString::number(m_maximumTime));
    writer->writeAttribute("averageTime", QString::number(m_averageTime));

    writer->writeAttribute("sentPackets", QString::number(m_sentPackets));
    writer->writeAttribute("receivedPackets", QString::number(m_receivedPackets));
    writer->writeEndElement();
}

QHostAddress Traceroute::hostAddress() const
{
    return m_hostAddress;
}

void Traceroute::setHostAddress(const QHostAddress &hostAddress)
{
    m_hostAddress = hostAddress;
}

void Traceroute::parse(const QDomElement &element)
{
    m_hostAddress = QHostAddress(element.attribute("hostAddress"));
    QDomElement pingElement = element.firstChildElement("ping");
    while (!pingElement.isNull())
    {
        Ping ping;
        ping.parse(pingElement);
        append(ping);
        pingElement = pingElement.nextSiblingElement("ping");
    }
}

void Traceroute::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("traceroute");
    writer->writeAttribute("hostAddress", m_hostAddress.toString());
    for (int i = 0; i < size(); ++i)
        at(i).toXml(writer);
    writer->writeEndElement();
}

Ping NetworkInfo::ping(const QHostAddress &host, int maxPackets)
{
    Ping info;
    info.setHostAddress(host);

    QString program;
    if (host.protocol() == QAbstractSocket::IPv4Protocol)
    {
        program = "ping";
    } else {
        program = "ping6";
    }

    QStringList arguments;
#ifdef Q_OS_WIN
    arguments << "-n" << QString::number(maxPackets);
#else
    arguments << "-c" << QString::number(maxPackets);
#endif
    arguments << host.toString();

    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    process.waitForFinished(60000);

    /* process stats */
    QString result = QString::fromLocal8Bit(process.readAllStandardOutput());

#ifdef Q_OS_WIN
    /* min max avg */
    QRegExp timeRegex(" = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms");
    if (timeRegex.indexIn(result))
    {
        info.setMinimumTime(timeRegex.cap(1).toInt());
        info.setMaximumTime(timeRegex.cap(2).toInt());
        info.setAverageTime(timeRegex.cap(3).toInt());
    }
    QRegExp packetRegex(" = ([0-9]+), [^ ]+ = ([0-9]+),");
#else
    /* min/avg/max/stddev */
    QRegExp timeRegex(" = ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms");
    if (timeRegex.indexIn(result))
    {
        info.setMinimumTime(timeRegex.cap(1).toFloat());
        info.setAverageTime(timeRegex.cap(2).toFloat());
        info.setMaximumTime(timeRegex.cap(3).toFloat());
    }
    /* Linux  : 2 packets transmitted, 2 received, 0% packet loss, time 1001ms */
    /* Mac OS : 2 packets transmitted, 1 packets received, 50.0% packet loss */
    QRegExp packetRegex("([0-9]+) [^ ]+ [^ ]+, ([0-9]+) [^ ]+( [^ ]+)?, [0-9]+");
#endif
    if (packetRegex.indexIn(result))
    {
        info.setSentPackets(packetRegex.cap(1).toInt());
        info.setReceivedPackets(packetRegex.cap(2).toInt());
    }
    return info;
}

Traceroute NetworkInfo::traceroute(const QHostAddress &host, int maxPackets, int maxHops)
{
    Traceroute hops;
    hops.setHostAddress(host);

    QString program;
    QStringList arguments;

    if (host.protocol() != QAbstractSocket::IPv4Protocol)
    {
        qWarning("IPv6 traceroute is not supported");
        return hops;
    } else {
#ifdef Q_OS_WIN
        program = "tracert";
#else
        program = "traceroute";
#endif
    }

#ifdef Q_OS_WIN
    if (maxPackets > 0)
        arguments << "-h" << QString::number(maxHops);
#else
    arguments << "-n";
    if (maxPackets > 0)
        arguments << "-q" << QString::number(maxPackets);
    if (maxHops > 0)
        arguments << "-m" << QString::number(maxHops);
#endif
    arguments << host.toString();

    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    process.waitForFinished(60000);

    /* process results */
    QString result = QString::fromLocal8Bit(process.readAllStandardOutput());

    /*
     * Windows :  1  6.839 ms  19.866 ms  1.134 ms 192.168.99.1
     * Windows :  2  <1 ms  <1 ms  <1 ms 192.168.99.1
     * *nix    :  1  192.168.99.1  6.839 ms  19.866 ms  1.134 ms
     *            2  192.168.99.1  6.839 ms * 1.134 ms
     */
    QRegExp hopRegex("\\s+[0-9]+\\s+(.+)");
    QRegExp ipRegex("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
    QRegExp timeRegex("([0-9.]+|<1)");
    foreach (const QString &line, result.split("\n"))
    {
        if (hopRegex.exactMatch(line))
        {
            Ping hop;
            float totalTime = 0.0;

            foreach (const QString &bit, hopRegex.cap(1).split(QRegExp("\\s+")))
            {
                if (ipRegex.exactMatch(bit))
                    hop.setHostAddress(QHostAddress(bit));
                else if (bit == "*")
                    hop.setSentPackets(hop.sentPackets() + 1);
                else if (timeRegex.exactMatch(bit))
                {
                    float hopTime = (bit == "<1") ? 0 : bit.toFloat();
                    hop.setSentPackets(hop.sentPackets() + 1);
                    hop.setReceivedPackets(hop.receivedPackets() + 1);
                    if (hopTime > hop.maximumTime())
                        hop.setMaximumTime(hopTime);
                    if (!hop.minimumTime() || hopTime < hop.minimumTime())
                        hop.setMinimumTime(hopTime);
                    totalTime += hopTime;
                }
            }
            if (hop.receivedPackets())
                hop.setAverageTime(totalTime / static_cast<float>(hop.receivedPackets()));
            hops.append(hop);
        }
    }
    return hops;
}


