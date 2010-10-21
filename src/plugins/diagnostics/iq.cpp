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

#include <QCoreApplication>
#include <QDateTime>
#include <QDomElement>
#include <QNetworkInterface>

#include "iq.h"

const char* ns_diagnostics = "http://wifirst.net/protocol/diagnostics";

QList<QHostInfo> DiagnosticsIq::lookups() const
{
    return m_lookups;
}

void DiagnosticsIq::setLookups(const QList<QHostInfo> &lookups)
{
    m_lookups = lookups;
}

QList<Ping> DiagnosticsIq::pings() const
{
    return m_pings;
}

void DiagnosticsIq::setPings(const QList<Ping> &pings)
{
    m_pings = pings;
}

QList<Software> DiagnosticsIq::softwares() const
{
    return m_softwares;
}

void DiagnosticsIq::setSoftwares(const QList<Software> &softwares)
{
    m_softwares = softwares;
}

QList<Traceroute> DiagnosticsIq::traceroutes() const
{
    return m_traceroutes;
}

void DiagnosticsIq::setTraceroutes(const QList<Traceroute> &traceroutes)
{
    m_traceroutes = traceroutes;
}

QList<Interface> DiagnosticsIq::interfaces() const
{
    return m_interfaces;
}

void DiagnosticsIq::setInterfaces(QList<Interface> &interfaces)
{
    m_interfaces = interfaces;
}

bool DiagnosticsIq::isDiagnosticsIq(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    return (queryElement.namespaceURI() == ns_diagnostics);
}

void DiagnosticsIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    QDomElement child = queryElement.firstChildElement();
    while (!child.isNull())
    {
        if (child.tagName() == QLatin1String("lookup"))
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
        else if (child.tagName() == QLatin1String("interface"))
        {
            Interface interface;
            interface.parse(child);
            m_interfaces << interface;
        }
        child = child.nextSiblingElement();
    }
}

void DiagnosticsIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
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
    foreach (const Ping &ping, m_pings)
        ping.toXml(writer);
    foreach (const Traceroute &traceroute, m_traceroutes)
        traceroute.toXml(writer);
    writer->writeEndElement();
}

