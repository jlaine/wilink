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

#ifndef __QNETIO_MIME_H__
#define __QNETIO_MIME_H__

#include <QByteArray>
#include <QHash>
#include <QHttpHeader>
#include <QStringList>

namespace QNetIO {

class MimeHeaders : public QHttpHeader
{
public:
    MimeHeaders();
    MimeHeaders(const QByteArray &data);
    virtual int majorVersion() const { return 1; };
    virtual int minorVersion() const { return 0; };
};

class MimePart
{
public:
    MimePart();
    MimePart(const QByteArray &data);
    QByteArray render() const;

public:
    MimeHeaders headers;
    QByteArray body;
};

class MimeMultipart
{
public:
    MimeMultipart();
    MimeMultipart(const QByteArray &data, const QByteArray &bound);
    QByteArray render() const;
    static QString randomString(int length = 32);

public:
    QByteArray boundary;
    QList<MimePart> parts;
    QByteArray preample;
    QByteArray epilogue;
};

class MimeForm : public MimeMultipart
{
public:
    MimeForm();
    MimeForm(const QByteArray &data, const QByteArray &bound);

    void addString(const QString &name, const QString &value);
    void addFile(const QString &name, const QString &filename, const QByteArray &data);
};

/** MimeTypes is used to manipulate MIME types.
 *
 * @ingroup Mime
 */
class MimeTypes
{
public:
    MimeTypes();

    QString guessType(const QString &filename);
    QStringList nameFilters(const QStringList &types);

private:
    QHash<QString, QStringList> db;
};

}

#endif
