/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#include <QCoreApplication>
#include <QDateTime>
#include <QDomElement>
#include <QNetworkInterface>

#include "QXmppDiagnosticIq.h"

const char* ns_diagnostics = "http://wifirst.net/protocol/diagnostics";

QList<Interface> QXmppDiagnosticIq::interfaces() const
{
    return m_interfaces;
}

void QXmppDiagnosticIq::setInterfaces(QList<Interface> &interfaces)
{
    m_interfaces = interfaces;
}

QList<QHostAddress> QXmppDiagnosticIq::nameServers() const
{
    return m_nameServers;
}

void QXmppDiagnosticIq::setNameServers(const QList<QHostAddress> &nameServers)
{
    m_nameServers = nameServers;
}

QList<QHostInfo> QXmppDiagnosticIq::lookups() const
{
    return m_lookups;
}

void QXmppDiagnosticIq::setLookups(const QList<QHostInfo> &lookups)
{
    m_lookups = lookups;
}

QList<Ping> QXmppDiagnosticIq::pings() const
{
    return m_pings;
}

void QXmppDiagnosticIq::setPings(const QList<Ping> &pings)
{
    m_pings = pings;
}

QList<Software> QXmppDiagnosticIq::softwares() const
{
    return m_softwares;
}

void QXmppDiagnosticIq::setSoftwares(const QList<Software> &softwares)
{
    m_softwares = softwares;
}

QList<Traceroute> QXmppDiagnosticIq::traceroutes() const
{
    return m_traceroutes;
}

void QXmppDiagnosticIq::setTraceroutes(const QList<Traceroute> &traceroutes)
{
    m_traceroutes = traceroutes;
}

QList<Transfer> QXmppDiagnosticIq::transfers() const
{
    return m_transfers;
}

void QXmppDiagnosticIq::setTransfers(const QList<Transfer> &transfers)
{
    m_transfers = transfers;
}

bool QXmppDiagnosticIq::isDiagnosticIq(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    return (queryElement.namespaceURI() == ns_diagnostics);
}

void QXmppDiagnosticIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    QDomElement child = queryElement.firstChildElement();
    while (!child.isNull())
    {
        if (child.tagName() == QLatin1String("interface"))
        {
            Interface interface;
            interface.parse(child);
            m_interfaces << interface;
        }
        else if (child.tagName() == QLatin1String("lookup"))
        {
            QHostInfo lookup;
            lookup.setHostName(child.attribute("hostName"));
            QList<QHostAddress> addresses;
            QDomElement addressElement = child.firstChildElement("address");
            while (!addressElement.isNull())
            {
                addresses.append(QHostAddress(addressElement.text()));
                addressElement = addressElement.nextSiblingElement("address");
            }
            lookup.setAddresses(addresses);
            m_lookups << lookup;
        }
        else if (child.tagName() == QLatin1String("nameServer"))
            m_nameServers.append(QHostAddress(child.attribute("address")));
        else if (child.tagName() == QLatin1String("ping"))
        {
            Ping ping;
            ping.parse(child);
            m_pings << ping;
        }
        else if (child.tagName() == QLatin1String("software"))
        {
            Software software;
            software.parse(child);
            m_softwares << software;
        }
        else if (child.tagName() == QLatin1String("traceroute"))
        {
            Traceroute traceroute;
            traceroute.parse(child);
            m_traceroutes << traceroute;
        }
        else if (child.tagName() == QLatin1String("transfer"))
        {
            Transfer transfer;
            transfer.parse(child);
            m_transfers << transfer;
        }
        child = child.nextSiblingElement();
    }
}

void QXmppDiagnosticIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("query");
    writer->writeAttribute("xmlns", ns_diagnostics);
    foreach (const Software &software, m_softwares)
        software.toXml(writer);
    foreach (const Interface &interface, m_interfaces)
        interface.toXml(writer);
    foreach (const QHostInfo &lookup, m_lookups)
    {
        writer->writeStartElement("lookup");
        writer->writeAttribute("hostName", lookup.hostName());
        foreach (const QHostAddress &address, lookup.addresses())
            writer->writeTextElement("address", address.toString());
        writer->writeEndElement();
    }
    foreach (const QHostAddress &server, m_nameServers) {
        writer->writeStartElement("nameServer");
        writer->writeAttribute("address", server.toString());
        writer->writeEndElement();
    }
    foreach (const Ping &ping, m_pings)
        ping.toXml(writer);
    foreach (const Traceroute &traceroute, m_traceroutes)
        traceroute.toXml(writer);
    foreach (const Transfer &transfer, m_transfers)
        transfer.toXml(writer);
    writer->writeEndElement();
}

