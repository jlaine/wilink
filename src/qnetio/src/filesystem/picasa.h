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

#ifndef __QNETIO_FILESYSTEM_PICASA_H__
#define __QNETIO_FILESYSTEM_PICASA_H__

#include <QHash>

#include "filesystem.h"

class QNetworkAccessManager;
class QNetworkReply;

using namespace QNetIO;

class QDomElement;

class FileMapper
{
public:
    void addPath(const QUrl &virtualUrl, const QUrl &realUrl);
    QUrl path(const QUrl &url);
    QPair<QUrl, QString> splitPath(const QUrl &url);

private:
    QHash<QString, QUrl> fileHash;
};

/** Support for access to Picasa photo shares as a file system.
 */
class PicasaFileSystem : public FileSystem
{
    Q_OBJECT

public:
    PicasaFileSystem(QObject *parent=NULL);

public slots:
    FileSystemJob *get(const QUrl &fileUrl, ImageSize type);
    FileSystemJob* open(const QUrl &url);
    FileSystemJob* list(const QUrl &dirUrl, const QString &filter = QString());
    FileSystemJob* mkdir(const QUrl &dirUrl);
    FileSystemJob* put(const QUrl &fileUrl, QIODevice *data);
    FileSystemJob* remove(const QUrl &fileUrl);
    FileSystemJob* rmdir(const QUrl &dirUrl);

private:
    FileInfo parseEntry(QDomElement &entry, const QUrl &baseUrl);
    FileSystemJob *removeEntry(FileSystemJob::Operation operation, const QUrl &url);

    FileMapper mapper;
    QNetworkAccessManager *network;
    QString authToken;

    QHash<QString, QString> fileHash;
    QHash<QString, QString> smallHash;
    QHash<QString, QString> mediumHash;
    QHash<QString, QString> editHash;

    friend class PicasaFileSystemJob;
};

#endif
