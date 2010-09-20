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

#include <QDebug>
#include <QRegExp>

#include "chat_utils.h"

static const qint64 KILO = 1000LL;
static const qint64 MEGA = 1000000LL;
static const qint64 GIGA = 1000000000LL;
static const qint64 TERA = 1000000000000LL;

bool isBareJid(const QString &jid)
{
    QRegExp jidValidator("^[^@/]+@[^@/]+$");
    return jidValidator.exactMatch(jid);
}

bool isFullJid(const QString &jid)
{
    QRegExp jidValidator("[^@/]+@[^@/]+/[^@/]+");
    return jidValidator.exactMatch(jid);
}

QString indentXml(const QString &xml)
{
    if (!xml.startsWith("<"))
        return xml;
    QRegExp expression("<([^>]+)>");
    int level = 0;
    int index = expression.indexIn(xml);
    QString output;
    while (index >= 0) {
        int length = expression.matchedLength();
        QString tagContents = expression.cap(1);

        if (tagContents.startsWith("/"))
            level--;

        if (output.endsWith("\n"))
            output += QString(level * 4, ' ');

        if (!tagContents.startsWith("?") &&
            !tagContents.endsWith("?") &&
            !tagContents.startsWith("/") &&
            !tagContents.endsWith("/"))
            level++;

        output += expression.cap(0);

        // look for next match
        int cursor = index + length;
        index = expression.indexIn(xml, cursor);
        if (index >= cursor)
        {
            QString data = xml.mid(cursor, index - cursor).trimmed();
            if (data.isEmpty())
                output += "\n";
            else
                output += data;
        }
    }
    return output;
}

QString sizeToString(qint64 size)
{
    if (size < KILO)
        return QString::fromUtf8("%1 B").arg(size);
    else if (size < MEGA)
        return QString::fromUtf8("%1 KB").arg(double(size) / double(KILO), 0, 'f', 1);
    else if (size < GIGA)
        return QString::fromUtf8("%1 MB").arg(double(size) / double(MEGA), 0, 'f', 1);
    else if (size < TERA)
        return QString::fromUtf8("%1 GB").arg(double(size) / double(GIGA), 0, 'f', 1);
    else
        return QString::fromUtf8("%1 TB").arg(double(size) / double(TERA), 0, 'f', 1);
}

QString speedToString(qint64 size)
{
    size *= 8;
    if (size < KILO)
        return QString::fromUtf8("%1 b/s").arg(size);
    else if (size < MEGA)
        return QString::fromUtf8("%1 Kb/s").arg(double(size) / double(KILO), 0, 'f', 1);
    else if (size < GIGA)
        return QString::fromUtf8("%1 Mb/s").arg(double(size) / double(MEGA), 0, 'f', 1);
    else if (size < TERA)
        return QString::fromUtf8("%1 Gb/s").arg(double(size) / double(GIGA), 0, 'f', 1);
    else
        return QString::fromUtf8("%1 Tb/s").arg(double(size) / double(TERA), 0, 'f', 1);
}

