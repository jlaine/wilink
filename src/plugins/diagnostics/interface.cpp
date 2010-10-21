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

#include "interface.h"

QList<QNetworkAddressEntry> Interface::addressEntries() const
{
    return m_addressEntries;
}

void Interface::setAddressEntries(const QList<QNetworkAddressEntry> &addressEntries)
{
    m_addressEntries = addressEntries;
}

QString Interface::name() const
{
    return m_name;
}

void Interface::setName(const QString &name)
{
    m_name = name;
}

QList<WirelessNetwork> Interface::wirelessNetworks() const
{
    return m_wirelessNetworks;
}

void Interface::setWirelessNetworks(const QList<WirelessNetwork> &wirelessNetworks)
{
    m_wirelessNetworks = wirelessNetworks;
}

WirelessStandards Interface::wirelessStandards() const
{
    return m_wirelessStandards;
}

void Interface::setWirelessStandards(WirelessStandards wirelessStandards)
{
    m_wirelessStandards = wirelessStandards;
}

void Interface::parse(const QDomElement &element)
{
    m_name = element.attribute("name");

    // addresses
    QDomElement addressElement = element.firstChildElement("address");
    while (!addressElement.isNull())
    {
        QNetworkAddressEntry entry;
        entry.setBroadcast(QHostAddress(addressElement.attribute("broadcast")));
        entry.setIp(QHostAddress(addressElement.attribute("ip")));
        entry.setNetmask(QHostAddress(addressElement.attribute("netmask")));
        m_addressEntries << entry;
        addressElement = addressElement.nextSiblingElement("address");
    }

    // wireless
    QDomElement wirelessElement = element.firstChildElement("wireless");
    m_wirelessStandards = WirelessStandards::fromString(wirelessElement.attribute("standards"));
    QDomElement networkElement = wirelessElement.firstChildElement("network");
    while (!networkElement.isNull())
    {
        WirelessNetwork network;
        network.parse(networkElement);
        m_wirelessNetworks << network;
        networkElement = networkElement.nextSiblingElement("network");
    }
}

void Interface::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("interface");
    writer->writeAttribute("name", m_name);

    // addresses
    foreach (const QNetworkAddressEntry &entry, m_addressEntries)
    {
        writer->writeStartElement("address");
        writer->writeAttribute("broadcast", entry.broadcast().toString());
        writer->writeAttribute("ip", entry.ip().toString());
        writer->writeAttribute("netmask", entry.netmask().toString());
        writer->writeEndElement();
    }

    // wireless
    if (m_wirelessStandards || !m_wirelessNetworks.isEmpty())
    {
        writer->writeStartElement("wireless");
        writer->writeAttribute("standards", m_wirelessStandards.toString());
        foreach (const WirelessNetwork &network, m_wirelessNetworks)
            network.toXml(writer);
        writer->writeEndElement();
    }
    writer->writeEndElement();
}
