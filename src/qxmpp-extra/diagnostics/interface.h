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

#ifndef __DIAGNOSTICS_INTERFACE_H__
#define __DIAGNOSTICS_INTERFACE_H__

#include "wireless.h"

/** The Interface class represents information about a network interface.
 */
class Interface
{
public:
    QList<QNetworkAddressEntry> addressEntries() const;
    void setAddressEntries(const QList<QNetworkAddressEntry> &addressEntries);

    QString name() const;
    void setName(const QString &name);

    QList<WirelessNetwork> wirelessNetworks() const;
    void setWirelessNetworks(const QList<WirelessNetwork> &wirelessNetworks);

    WirelessStandards wirelessStandards() const;
    void setWirelessStandards(WirelessStandards wirelessStandards);

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

private:
    QList<QNetworkAddressEntry> m_addressEntries;
    QString m_name;
    QList<WirelessNetwork> m_wirelessNetworks;
    WirelessStandards m_wirelessStandards;
};

#endif
