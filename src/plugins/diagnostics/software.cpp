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

#include "software.h"

QString Software::name() const
{
    return m_name;
}

void Software::setName(const QString &name)
{
    m_name = name;
}

QString Software::type() const
{
    return m_type;
}

void Software::setType(const QString &type)
{
    m_type = type;
}

QString Software::version() const
{
    return m_version;
}

void Software::setVersion(const QString &version)
{
    m_version = version;
}

void Software::parse(const QDomElement &element)
{
    m_name = element.attribute("name");
    m_type = element.attribute("type");
    m_version = element.attribute("version");
}

void Software::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("software");
    writer->writeAttribute("type", m_type);
    writer->writeAttribute("name", m_name);
    writer->writeAttribute("version", m_version);
    writer->writeEndElement();
}

