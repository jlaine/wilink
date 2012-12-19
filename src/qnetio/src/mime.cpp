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

#include "mime.h"

using namespace QNetIO;

typedef struct
{
    const char *type;
    const char *extensions;
} MimeEntry;

MimeHeaders::MimeHeaders()
{
}

MimeHeaders::MimeHeaders(const QByteArray &data)
    : QHttpHeader(data)
{
}

MimePart::MimePart()
{
}

MimePart::MimePart(const QByteArray &data)
{
    QByteArray lines;

    /* Read headers */
    int pos = -1;

    while (1)
    {
        if ((pos = data.indexOf('\n', pos + 1)) < 0)
        {
            qWarning("buffer does not contain end of line");
            return;
        }

        if (!pos || data[pos-1] == '\n' || (data[pos-1] == '\r' && data[pos-2] == '\n'))
            break;
    }
    pos++;

    headers = MimeHeaders(data.left(pos));
    body = data.mid(pos);
    if (body.endsWith('\n')) body.chop(1);
    if (body.endsWith('\r')) body.chop(1);

/*
    qDebug("--");
    qDebug(headers.toString().toLocal8Bit());
    qDebug("--");
    qDebug(body);
*/
}

QByteArray MimePart::render() const
{
    return headers.toString().toAscii() + "\r\n" +  body;
}

MimeMultipart::MimeMultipart()
{
    boundary = "-------------------------------" + randomString().toAscii();
}

MimeMultipart::MimeMultipart(const QByteArray &data, const QByteArray &bound)
    : boundary(bound)
{
    QByteArray separator("--" + boundary);
    int pos = 0;

    if ((pos = data.indexOf(separator)) < 0)
    {
        qWarning("buffer does not contain boundary");
        return;
    }
    preample = data.left(pos);

    while (1)
    {
        int next;
        bool final = (data.mid(pos + separator.size(), 2) == "--");

        if ((pos = data.indexOf('\n', pos)) < 0)
        {
            qWarning("no end of line after boundary");
            return;
        }

        if (final)
            break;

        if ((next = data.indexOf(separator, ++pos)) < 0)
        {
            qWarning("no next boundary");
            return;
        }

        parts.append(MimePart(data.mid(pos, next - pos)));
        pos = next;
    }
    pos++;
    epilogue = data.mid(pos);
}

QString MimeMultipart::randomString(int length)
{
    QString output;
    static bool initialised = false;

    if (!initialised)
    {
        qsrand((uint)QDateTime::currentDateTime().toTime_t());
        initialised = true;
    }

    for( int i = 0 ; i < length ; ++i )
    {
        int number = qrand() % 122;
        if( 48 > number )
            number += 48;
        if( ( 57 < number ) && ( 65 > number ) )
            number += 7;
        if( ( 90 < number ) && ( 97 > number ) )
            number += 6;
        output.append((char)number);
    }
    return output;
}

QByteArray MimeMultipart::render() const
{
    QList<MimePart>::const_iterator iter;
    QByteArray buffer;

    for (iter = parts.constBegin(); iter != parts.constEnd(); iter++)
    {
        buffer.append("--" + boundary + "\r\n");
        buffer.append(iter->render() + "\r\n");
    }
    buffer.append("--" + boundary + "--\r\n");
    return buffer;
}

MimeForm::MimeForm()
{
}

MimeForm::MimeForm(const QByteArray &data, const QByteArray &bound)
    : MimeMultipart(data, bound)
{
}

void MimeForm::addString(const QString &name, const QString &value)
{
    MimePart part;
    QByteArray data = value.toUtf8();
    QString disposition = "form-data";
    if (!name.isEmpty())
        disposition += "; name=\"" + name + "\"";
    part.headers.setValue("Content-Disposition", disposition);
    part.headers.setValue("Content-Length", QString::number(data.size()));
    part.headers.setValue("Content-Type", "text/plain; charset=utf-8");
    part.headers.setValue("Content-Transfer-Encoding", "binary");
    part.body = data;
    parts.append(part);
}

void MimeForm::addFile(const QString &name, const QString &filename, const QByteArray &data)
{
    MimePart part;
    QString disposition = "form-data";
    if (!name.isEmpty())
        disposition += "; name=\"" + name + "\"";
    if (!filename.isEmpty())
        disposition += "; filename=\"" + QFileInfo(filename).fileName() + "\"";
    part.headers.setValue("Content-Disposition", disposition);
    part.headers.setValue("Content-Length", QString::number(data.size()));
    part.headers.setValue("Content-Type", MimeTypes().guessType(filename));
    part.headers.setValue("Content-Transfer-Encoding", "binary");
    part.body = data;
    parts.append(part);
}

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

