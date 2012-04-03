/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __WILINK_PHOTOS_H__
#define __WILINK_PHOTOS_H__

#include <QDeclarativeImageProvider>
#include <QSet>
#include <QUrl>

#include "filesystem.h"

#include "model.h"

using namespace QNetIO;

class PhotoCachePrivate;
class PhotoDownloadItem;
class PhotoQueueItem;
class PhotoQueueModel;
class PhotoResizer;
class QThread;

class PhotoCache : public QObject
{
    Q_OBJECT

public:
    static PhotoCache *instance();
    bool imageReady(const QUrl &url, FileSystem::ImageSize size) const;
    QUrl imageUrl(const FileInfo &info, FileSystem::ImageSize size, FileSystem *fs);

signals:
    void photoChanged(const QUrl &url, FileSystem::ImageSize size);

private slots:
    void _q_jobFinished();

private:
    PhotoCache();
    void processQueue();

    QSet<FileSystem*> m_fileSystems;
    QList<PhotoDownloadItem*> m_downloadQueue;
    PhotoDownloadItem *m_downloadItem;
    FileSystemJob *m_downloadJob;
};

class PhotoImageProvider : public QDeclarativeImageProvider
{
public:
    PhotoImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

class PhotoModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(bool canCreateAlbum READ canCreateAlbum NOTIFY permissionsChanged)
    Q_PROPERTY(bool canUpload READ canUpload NOTIFY permissionsChanged)
    Q_PROPERTY(bool showFiles READ showFiles WRITE setShowFiles NOTIFY showFilesChanged)
    Q_PROPERTY(QUrl rootUrl READ rootUrl WRITE setRootUrl NOTIFY rootUrlChanged)
    Q_PROPERTY(PhotoQueueModel* uploads READ uploads CONSTANT)

public:
    PhotoModel(QObject *parent = 0);

    QUrl rootUrl() const;
    void setRootUrl(const QUrl &rootUrl);

    bool showFiles() const;
    void setShowFiles(bool show);

    bool canCreateAlbum() const;
    bool canUpload() const;
    PhotoQueueModel *uploads() const;

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void permissionsChanged();
    void rootUrlChanged(const QUrl &rootUrl);
    void showFilesChanged();

public slots:
    void createAlbum(const QString &name);
    void refresh();
    bool removeRow(int row);
    void upload(const QString &filePath);

private slots:
    void _q_jobFinished(FileSystemJob *job);
    void _q_photoChanged(const QUrl &url, FileSystem::ImageSize size);

private:
    FileSystem *m_fs;
    QMap<QString,FileSystem*> m_fileSystems;
    FileSystemJob::Operations m_permissions;
    QUrl m_rootUrl;
    bool m_showFiles;
    PhotoQueueModel *m_uploads;
};

class PhotoQueueModel : public ChatModel
{
    Q_OBJECT

public:
    enum Role {
        IsDirRole = ChatModel::UserRole,
        SpeedRole,
        DoneBytesRole,
        DoneFilesRole,
        TotalBytesRole,
        TotalFilesRole,
    };

    PhotoQueueModel(QObject *parent = 0);
    ~PhotoQueueModel();

    void append(const QString &sourcePath, FileSystem *fileSystem, const QString &destinationPath);

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public slots:
    void cancel(int row);

private slots:
    void _q_jobFinished();
    void _q_uploadProgress(qint64 done, qint64 total);
    void _q_uploadResized(QIODevice *device);

private:
    void processQueue();

    PhotoModel *m_photoModel;
    PhotoResizer *m_resizer;
    QThread *m_resizerThread;
    QIODevice *m_uploadDevice;
    PhotoQueueItem *m_uploadItem;
};

class PhotoResizer : public QObject
{
    Q_OBJECT

public:
    PhotoResizer(QObject *parent = 0);

signals:
    void finished(QIODevice *device);

public slots:
    void resize(const QString &path);
};

#endif
