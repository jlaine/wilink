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
#include "wireless.h"

void WirelessNetwork::parse(const QDomElement &element)
{
    w_isCurrent = element.attribute("current") == QLatin1String("1");
    w_cinr = element.attribute("cinr").toInt();
    w_rssi = element.attribute("rssi").toInt();
    w_ssid = element.attribute("ssid");
}

void WirelessNetwork::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("network");
    if (w_isCurrent)
        writer->writeAttribute("current", "1");
    writer->writeAttribute("cinr", QString::number(w_cinr));
    writer->writeAttribute("rssi", QString::number(w_rssi));
    writer->writeAttribute("ssid", w_ssid);
    writer->writeEndElement();
}

