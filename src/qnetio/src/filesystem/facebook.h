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

#ifndef __QNETIO_FILESYSTEM_FACEBOOK_H__
#define __QNETIO_FILESYSTEM_FACEBOOK_H__

#include <QMap>
#include <QHash>

#include "filesystem.h"

class QNetworkAccessManager;
class QNetworkReply;

using namespace QNetIO;

/** Support for access to Facebook photo shares as a file system.
 */
class FacebookFileSystem : public FileSystem
{
    Q_OBJECT

public:
    FacebookFileSystem(QObject *parent=NULL);

public slots:
    FileSystemJob *get(const QUrl &fileUrl, ImageSize type);
    FileSystemJob* open(const QUrl &url);
    FileSystemJob* list(const QUrl &dirUrl, const QString &filter = QString());
    FileSystemJob* mkdir(const QUrl &dirUrl);
    FileSystemJob* put(const QUrl &fileUrl, QIODevice *data);

protected:
    void call(const QString &method, QMap<QString,QString> params);
    FileInfoList handleList(QNetworkReply *reply);
    void handleOpen(QNetworkReply *reply);

protected slots:
    void onFinished(QNetworkReply *reply);

protected:
    QNetworkAccessManager *network;
    QString authToken;
    int callId;
    int lastCommand;
    QString sessionKey;
    QString secret;
    QString uid;

    // path mapping
    QString baseVirtual;
    QHash<QString, QString> albumHash;
    QHash<QString, QString> reversedAlbumHash;
    QHash<QString, QString> photoHash;
};

#endif

