/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include "mimetypes.h"

using namespace QNetIO;

typedef struct
{
    const char *type;
    const char *extensions;
} MimeEntry;

static MimeEntry MIME_TYPES[] = {
    { "application/pdf", "pdf" },
    { "application/x-apple-diskimage", "dmg" },
    { "application/zip", "zip" },
    { "audio/mpeg", "mp3" },
    { "audio/mp4", "m4p" },
    { "audio/ogg", "ogg" },
    { "audio/x-wav", "wav" },
    { "image/gif", "gif" },
    { "image/jpeg", "jpeg jpg jpe" },
    { "image/png", "png" },
    { "text/css", "css" },
    { "text/html", "html" },
    { "text/plain", "txt" },
    { "video/x-msvideo", "avi" },
    { "video/mpeg", "mpeg mpg m4v" },
    { "video/quicktime", "mov" },
    { NULL, NULL },
};

/** Construct a new MimeTypes instance and load all known types.
 */
MimeTypes::MimeTypes()
{
    for (MimeEntry *entry = MIME_TYPES; entry->type; entry++)
        db.insert(entry->type, QString::fromAscii(entry->extensions).split(" "));

    // FIXME: get additional entries from /etc/mime.types or equivalent
#if 0
    QFile types("/etc/mime.types");
    if (types.open(QIODevice::ReadOnly))
    {
        QStringList lines = QString(types.readAll()).split('\n');
        for (int i  = 0; i < lines.size(); i++)
        {
            if (!lines[i].isEmpty() && !lines[i].startsWith('#'))
            {
                QStringList bits = lines[i].split(QRegExp("\\s+"));
                QString type = bits.takeFirst();
                if (bits.size() > 0)
                    db.insert(type, bits);
            }
        }
    }
#endif
}

/** Guess the MIME type for the given file name.
 *
 * @param filename
 */
QString MimeTypes::guessType(const QString &filename)
{
    QHash<QString, QStringList>::const_iterator iter;
    QString extension = QFileInfo(filename).suffix();

    for (iter = db.constBegin(); iter != db.constEnd(); iter++)
    {
        if (iter.value().contains(extension, Qt::CaseInsensitive))
            return iter.key();
    }
    return "application/octet-stream";
}

/** Return a list of file extension filters for the given MIME types.
 *
 *  This list can then be used to filter files during a search for instance.
 *
 * @param types
 */
QStringList MimeTypes::nameFilters(const QStringList &types)
{
    QStringList filters;
    for (int i = 0; i < types.size(); i++)
    {
        QStringList extensions = db.value(types[i], QStringList());
        for (int j = 0; j < extensions.size(); j++)
            filters << ("*." + extensions[j]);
    }
    return filters;
}

