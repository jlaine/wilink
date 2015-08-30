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

#ifndef __QNETIO_FILESYSTEM_MTP_H__
#define __QNETIO_FILESYSTEM_MTP_H__

#include "filesystem.h"

typedef struct _Camera Camera;
typedef struct _GPContext GPContext;

using namespace QNetIO;

/** Support for access to Media Transfer Protocol devices as a file system.
 */
class GPhotoFileSystem : public FileSystem
{
    Q_OBJECT

public:
    GPhotoFileSystem(QObject *parent=NULL);
    ~GPhotoFileSystem();

public slots:
    FileSystemJob *get(const QUrl &fileUrl, ImageSize type);
    FileSystemJob* open(const QUrl &url);
    FileSystemJob* list(const QUrl &dirUrl, const QString &filter = QString());

private:
    Camera *camera;
    GPContext *context;
};

class GPhotoFileSystemPlugin : public QNetIO::FileSystemPlugin
{
    Q_OBJECT

public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent);
};

#endif
