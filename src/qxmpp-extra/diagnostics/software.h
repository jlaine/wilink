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

#ifndef __DIAGNOSTICS_SOFTWARE_H__
#define __DIAGNOSTICS_SOFTWARE_H__

#include <QXmlStreamWriter>

class QDomElement;

class Software
{
public:
    QString type() const;
    void setType(const QString &type);

    QString name() const;
    void setName(const QString &name);

    QString version() const;
    void setVersion(const QString &version);

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

private:
    QString m_type;
    QString m_name;
    QString m_version;
};

#endif
