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

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCache>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLayout>
#include <QThread>

#include "QXmppClient.h"

#include "application.h"
#include "declarative.h"
#include "photos.h"

static const QSize UPLOAD_SIZE(2048, 2048);

static PhotoCache *photoCache = 0;
static QCache<QString, QImage> photoImageCache;
static bool photoInitialised = false;

class PhotoNetworkAccessManagerFactory : public FileSystemNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent)
    {
        return new NetworkAccessManager(parent);
    };
};

class PhotoDownloadItem
{
public:
    FileSystem *fs;
    FileSystem::ImageSize type;
    QUrl url;
};

enum PhotoRole
{
    ImageRole = ChatModel::UserRole,
    ImageReadyRole,
    IsDirRole,
    SizeRole,
    UrlRole,
};

static bool isImage(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    const QString extension = fileName.split(".").last().toLower();
    return (extension == QLatin1String("gif") ||
            extension == QLatin1String("jpg") ||
            extension == QLatin1String("jpeg") ||
            extension == QLatin1String("png"));
}

PhotoCache::PhotoCache()
    : m_downloadItem(0),
    m_downloadJob(0)
{
}

/** When a download finishes, process the results.
 */
void PhotoCache::_q_jobFinished()
{
    Q_ASSERT(m_downloadItem);
    Q_ASSERT(m_downloadJob);

    if (m_downloadJob->error() == FileSystemJob::NoError) {
        // load image
        QImage *image = new QImage;
        image->load(m_downloadJob->data(), NULL);
        photoImageCache.insert(QString::number(m_downloadItem->type) + m_downloadItem->url.toString(), image);
        emit photoChanged(m_downloadItem->url, m_downloadItem->type);
    }

    m_downloadJob->deleteLater();
    m_downloadJob = 0;
    delete m_downloadItem;
    m_downloadItem = 0;

    processQueue();
}

bool PhotoCache::imageReady(const QUrl &url, FileSystem::ImageSize type) const
{
    const QString key = QString::number(type) + url.toString();
    return photoImageCache.contains(key);
}

QUrl PhotoCache::imageUrl(const QUrl &url, FileSystem::ImageSize type, FileSystem *fs)
{
    const QString key = QString::number(type) + url.toString();
    if (photoImageCache.contains(key)) {
        QUrl cacheUrl("image://photo");
        cacheUrl.setPath("/" + key);
        return cacheUrl;
    }

    // check if the url is already queued
    PhotoDownloadItem *job = 0;
    foreach (PhotoDownloadItem *ptr, m_downloadQueue) {
        if (ptr->url == url && ptr->type == type) {
            job = ptr;
            break;
        }
    }
    if (!job) {
        job = new PhotoDownloadItem;
        job->fs = fs;
        job->type = type;
        job->url = url;
        m_downloadQueue.append(job);
        processQueue();
    }

    // try returning a lower resolution
    for (int i = type - 1; i >= FileSystem::SmallSize; --i) {
        const QString key = QString::number(i) + url.toString();
        if (photoImageCache.contains(key)) {
            QUrl cacheUrl("image://photo");
            cacheUrl.setPath("/" + key);
            return cacheUrl;
        }
    }
    return wApp->qmlUrl("128x128/file.png");
}

PhotoCache *PhotoCache::instance()
{
    if (!photoCache)
        photoCache = new PhotoCache;
    return photoCache;
}

void PhotoCache::processQueue()
{
    if (m_downloadJob || m_downloadQueue.isEmpty())
        return;

    PhotoDownloadItem *job = m_downloadQueue.takeFirst();
    if (!m_fileSystems.contains(job->fs)) {
        m_fileSystems << job->fs;
    }
    m_downloadItem = job;
    m_downloadJob = job->fs->get(job->url, job->type);
    connect(m_downloadJob, SIGNAL(finished()),
            this, SLOT(_q_jobFinished()));
}

PhotoImageProvider::PhotoImageProvider()
    : QDeclarativeImageProvider(Image)
{
}

QImage PhotoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;
    QImage *cached = photoImageCache.object(id);
    if (cached) {
        image = *cached;
    } else {
        qWarning("Could not get photo for %s", qPrintable(id));
        image = QImage(":/128x128/file.png");
    }

    if (requestedSize.isValid())
        image = image.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);

    if (size)
        *size = image.size();
    return image;
}

class PhotoItem : public ChatModelItem, public FileInfo
{
public:
    PhotoItem(const FileInfo &info) : FileInfo(info) {};
};

PhotoModel::PhotoModel(QObject *parent)
    : ChatModel(parent),
    m_fs(0),
    m_permissions(FileSystemJob::None)
{
    bool check;
    Q_UNUSED(check);

    QHash<int, QByteArray> names = roleNames();
    names.insert(ImageRole, "image");
    names.insert(ImageReadyRole, "imageReady");
    names.insert(IsDirRole, "isDir");
    names.insert(SizeRole, "size");
    names.insert(UrlRole, "url");
    setRoleNames(names);

    m_uploads = new PhotoQueueModel(this);

    check = connect(PhotoCache::instance(), SIGNAL(photoChanged(QUrl,FileSystem::ImageSize)),
                    this, SLOT(_q_photoChanged(QUrl,FileSystem::ImageSize)));
    Q_ASSERT(check);
}

QVariant PhotoModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(m_fs);
    PhotoItem *item = static_cast<PhotoItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        if (item->isDir()) {
            return wApp->qmlUrl("128x128/album.png");
        } else {
            return PhotoCache::instance()->imageUrl(item->url(), FileSystem::SmallSize, m_fs);
        }
    }
    else if (role == ImageRole) {
        if (item->isDir()) {
            return wApp->qmlUrl("128x128/album.png");
        } else {
            return PhotoCache::instance()->imageUrl(item->url(), FileSystem::LargeSize, m_fs);
        }
    }
    else if (role == ImageReadyRole)
        return PhotoCache::instance()->imageReady(item->url(), FileSystem::LargeSize);
    else if (role == IsDirRole)
        return item->isDir();
    else if (role == NameRole)
        return item->name();
    else if (role == SizeRole)
        return item->size();
    else if (role == UrlRole)
        return item->url();
    return QVariant();
}

bool PhotoModel::canCreateAlbum() const
{
    return m_permissions & FileSystemJob::Mkdir;
}

bool PhotoModel::canUpload() const
{
    return m_permissions & FileSystemJob::Put;
}

void PhotoModel::createAlbum(const QString &name)
{
    if (name.isEmpty())
        return;

    QUrl newUrl = m_rootUrl;
    newUrl.setPath(newUrl.path() + "/" + name);
    m_fs->mkdir(newUrl);
}

/** Refresh the contents of the current folder.
 */
void PhotoModel::refresh()
{
    removeRows(0, rootItem->children.size());
    m_fs->list(m_rootUrl);
}

QUrl PhotoModel::rootUrl() const
{
    return m_rootUrl;
}

void PhotoModel::setRootUrl(const QUrl &rootUrl)
{
    bool check;
    Q_UNUSED(check);

    if (rootUrl == m_rootUrl)
        return;

    m_rootUrl = rootUrl;
    m_fs = m_fileSystems.value(rootUrl.scheme());
    if (m_fs) {
        refresh();
    } else {
        if (!photoInitialised) {
            FileSystem::setNetworkAccessManagerFactory(new PhotoNetworkAccessManagerFactory);
            photoInitialised = true;
        }

        m_fs = FileSystem::create(m_rootUrl, this);
        if (m_fs) {
            check = connect(m_fs, SIGNAL(jobFinished(FileSystemJob*)),
                            this, SLOT(_q_jobFinished(FileSystemJob*)));
            Q_ASSERT(check);

            m_fileSystems.insert(m_rootUrl.scheme(), m_fs);
            m_fs->open(m_rootUrl);
        } else {
            removeRows(0, rootItem->children.size());
        }
    }
    emit rootUrlChanged(m_rootUrl);
}

bool PhotoModel::removeRow(int row)
{
    if (row < 0 || row >= rootItem->children.size())
        return false;

    PhotoItem *item = static_cast<PhotoItem*>(rootItem->children.at(row));
    if (item->isDir())
        m_fs->rmdir(item->url());
    else
        m_fs->remove(item->url());
    return true;
}

void PhotoModel::upload(const QString &filePath)
{
    QString base = m_rootUrl.toString();
    while (base.endsWith("/"))
        base.chop(1);

    m_uploads->append(filePath, m_fs, base + "/" + QFileInfo(filePath).fileName());
}

PhotoQueueModel *PhotoModel::uploads() const
{
    return m_uploads;
}

/** When a command finishes, process its results.
 *
 * @param job
 */
void PhotoModel::_q_jobFinished(FileSystemJob *job)
{
    Q_ASSERT(m_fs);

    if (job->error() != FileSystemJob::NoError) {
        qWarning() << job->operationName() << "command failed";
        return;
    }

    switch (job->operation())
    {
    case FileSystemJob::Open:
    case FileSystemJob::Mkdir:
    case FileSystemJob::Put:
    case FileSystemJob::Remove:
    case FileSystemJob::Rmdir:
        refresh();
        break;
    case FileSystemJob::List:
        m_permissions = job->allowedOperations();
        emit permissionsChanged();

        removeRows(0, rootItem->children.size());
        foreach (const FileInfo& info, job->results()) {
            if (info.isDir() || isImage(info.name())) {
                PhotoItem *item = new PhotoItem(info);
                addItem(item, rootItem);
            }
        }
        break;
    default:
        break;
    }
}

/** When a photo changes, emit notifications.
 */
void PhotoModel::_q_photoChanged(const QUrl &url, FileSystem::ImageSize size)
{
    Q_UNUSED(size);
    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoItem *item = static_cast<PhotoItem*>(ptr);
        if (item->url() == url) {
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
}

class PhotoQueueItem : public ChatModelItem
{
public:
    PhotoQueueItem();

    QString sourcePath;
    QString destinationPath;
    FileSystem *fileSystem;
    bool finished;
    FileSystemJob *job;

    qint64 doneBytes;
    qint64 totalBytes;
};

PhotoQueueItem::PhotoQueueItem()
    : fileSystem(0),
    job(0),
    doneBytes(0),
    totalBytes(0)
{
}

PhotoQueueModel::PhotoQueueModel(QObject *parent)
    : ChatModel(parent),
    m_resizer(0),
    m_resizerThread(0),
    m_uploadDevice(0),
    m_uploadItem(0)
{
    // set role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(IsDirRole, "isDir");
    names.insert(SpeedRole, "speed");
    names.insert(DoneBytesRole, "doneBytes");
    names.insert(DoneFilesRole, "doneFiles");
    names.insert(TotalBytesRole, "totalBytes");
    names.insert(TotalFilesRole, "totalFiles");
    setRoleNames(names);

    m_resizerThread = new QThread;
    m_resizerThread->start();

    m_resizer = new PhotoResizer;
    m_resizer->moveToThread(m_resizerThread);
    connect(m_resizer, SIGNAL(finished(QIODevice*)),
            this, SLOT(_q_uploadResized(QIODevice*)));
}

PhotoQueueModel::~PhotoQueueModel()
{
    m_resizerThread->quit();
    m_resizerThread->wait();
    delete m_resizer;
    delete m_resizerThread;
}

void PhotoQueueModel::append(const QString &filePath, FileSystem *fileSystem, const QString &destinationPath)
{
    PhotoQueueItem *item = new PhotoQueueItem;
    item->sourcePath = filePath;
    item->destinationPath = destinationPath;
    item->fileSystem = fileSystem;
    addItem(item, rootItem);

    processQueue();
}

void PhotoQueueModel::cancel(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    PhotoQueueItem *item = static_cast<PhotoQueueItem*>(rootItem->children.at(row));
    if (item->job)
        item->job->abort();
    else
        removeRow(row);
}

QVariant PhotoQueueModel::data(const QModelIndex &index, int role) const
{
    PhotoQueueItem *item = static_cast<PhotoQueueItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        // FIXME: using a thumbnail for large pictures
        // is a total performance killer
        //return QUrl::fromLocalFile(item->sourcePath);
        return wApp->qmlUrl("file.png");
    } else if (role == NameRole) {
        return QFileInfo(item->sourcePath).fileName();
    } else if (role == SpeedRole) {
        // FIXME: implement speed calculation
        return 0;
    } else if (role == DoneBytesRole) {
        return item->doneBytes;
    } else if (role == DoneFilesRole) {
        return 0;
    } else if (role == TotalBytesRole) {
        return item->totalBytes;
    } else if (role == TotalFilesRole) {
        return 1;
    }
    return QVariant();
}

void PhotoQueueModel::processQueue()
{
    if (m_uploadItem)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
        if (!item->finished) {
            m_uploadItem = item;
            QMetaObject::invokeMethod(m_resizer, "resize", Q_ARG(QString, item->sourcePath));
            break;
        }
    }
}

/** When an upload finishes, process the results.
 */
void PhotoQueueModel::_q_jobFinished()
{
    if (m_uploadItem) {
        removeItem(m_uploadItem);
        m_uploadItem = 0;
    }

    if (m_uploadDevice) {
        delete m_uploadDevice;
        m_uploadDevice = 0;
    }

    processQueue();
}

/** When upload progress changes, emit notifications.
 */
void PhotoQueueModel::_q_uploadProgress(qint64 done, qint64 total)
{
    if (m_uploadItem) {
        m_uploadItem->doneBytes = done;
        m_uploadItem->totalBytes = total;
        emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
    }
}

void PhotoQueueModel::_q_uploadResized(QIODevice *device)
{
    if (m_uploadItem) {
        m_uploadDevice = device;
        m_uploadItem->job = m_uploadItem->fileSystem->put(m_uploadItem->destinationPath, m_uploadDevice);
        connect(m_uploadItem->job, SIGNAL(finished()),
                this, SLOT(_q_jobFinished()));
        connect(m_uploadItem->job, SIGNAL(uploadProgress(qint64,qint64)),
                this, SLOT(_q_uploadProgress(qint64,qint64)));
    }
}

PhotoResizer::PhotoResizer(QObject *parent)
    : QObject(parent)
{
}

void PhotoResizer::resize(const QString &path)
{
    QIODevice *device = 0;

    // process the next file to upload
    const QByteArray imageFormat = QImageReader::imageFormat(path);
    QImage image;
    if (!imageFormat.isEmpty() && image.load(path, imageFormat.constData()))
    {
        if (image.width() > UPLOAD_SIZE.width() || image.height() > UPLOAD_SIZE.height())
            image = image.scaled(UPLOAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        device = new QBuffer;
        device->open(QIODevice::WriteOnly);
        image.save(device, imageFormat.constData());
        device->open(QIODevice::ReadOnly);
    } else {
        device = new QFile(path);
        device->open(QIODevice::ReadOnly);
    }
    emit finished(device);
}

