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

static QString availableFilePath(const QString &dirPath, const QString &name)
{
    QString fileName = name;
    QDir downloadsDir(dirPath);
    if (downloadsDir.exists(fileName))
    {
        const QString fileBase = QFileInfo(fileName).completeBaseName();
        const QString fileSuffix = QFileInfo(fileName).suffix();
        int i = 2;
        while (downloadsDir.exists(fileName))
        {
            fileName = QString("%1_%2").arg(fileBase, QString::number(i++));
            if (!fileSuffix.isEmpty())
                fileName += "." + fileSuffix;
        }
    }
    return downloadsDir.absoluteFilePath(fileName);
}

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
        image->load(m_downloadJob, NULL);
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

QUrl PhotoCache::imageUrl(const FileInfo &info, FileSystem::ImageSize type, FileSystem *fs)
{
    if (info.isDir())
        return wApp->qmlUrl("128x128/album.png");

    const QString mimeType = info.mimeType();
    if (mimeType.startsWith("audio/"))
        return wApp->qmlUrl("128x128/audio-x-generic.png");
    else if (mimeType.startsWith("text/"))
        return wApp->qmlUrl("128x128/text-x-generic.png");
    else if (mimeType.startsWith("video/"))
        return wApp->qmlUrl("128x128/video-x-generic.png");
    else if (!mimeType.startsWith("image/"))
        return wApp->qmlUrl("128x128/file.png");

    const QUrl url = info.url();
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
    return wApp->qmlUrl("128x128/image-x-generic.png");
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

class FolderModelItem : public ChatModelItem, public FileInfo
{
public:
    FolderModelItem(const FileInfo &info) : FileInfo(info) {};
};

class FolderModelPrivate
{
public:
    FolderModelPrivate(FolderModel *qq);

    QString filter;
    FileSystem *fs;
    FileSystemJob *listJob;
    QMap<QString,FileSystem*> fileSystems;
    FileSystemJob::Operations permissions;
    FolderQueueModel *queue;
    QUrl rootUrl;
    bool showFiles;

private:
    FolderModel *q;
};

FolderModelPrivate::FolderModelPrivate(FolderModel *qq)
    : fs(0)
    , listJob(0)
    , permissions(FileSystemJob::None)
    , queue(0)
    , showFiles(true)
    , q(qq)
{
}

FolderModel::FolderModel(QObject *parent)
    : ChatModel(parent)
{
    bool check;
    Q_UNUSED(check);

    d = new FolderModelPrivate(this);
    d->queue = new FolderQueueModel(this);

    // watch image changes
    check = connect(PhotoCache::instance(), SIGNAL(photoChanged(QUrl,FileSystem::ImageSize)),
                    this, SLOT(_q_photoChanged(QUrl,FileSystem::ImageSize)));
    Q_ASSERT(check);

    // set role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(ImageRole, "image");
    names.insert(ImageReadyRole, "imageReady");
    names.insert(IsDirRole, "isDir");
    names.insert(SizeRole, "size");
    names.insert(UrlRole, "url");
    setRoleNames(names);
}

FolderModel::~FolderModel()
{
    delete d;
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(m_fs);
    FolderModelItem *item = static_cast<FolderModelItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        return PhotoCache::instance()->imageUrl(*item, FileSystem::SmallSize, d->fs);
    }
    else if (role == ImageRole) {
        return PhotoCache::instance()->imageUrl(*item, FileSystem::LargeSize, d->fs);
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

bool FolderModel::canCreateAlbum() const
{
    return d->permissions & FileSystemJob::Mkdir;
}

bool FolderModel::canUpload() const
{
    return d->permissions & FileSystemJob::Put;
}

void FolderModel::createAlbum(const QString &name)
{
    if (name.isEmpty())
        return;

    QUrl newUrl = d->rootUrl;
    newUrl.setPath(newUrl.path() + "/" + name);
    d->fs->mkdir(newUrl);
}

void FolderModel::download(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    FolderModelItem *item = static_cast<FolderModelItem*>(rootItem->children.at(row));
    d->queue->download(d->fs, *item, d->filter);
}

QString FolderModel::filter() const
{
    return d->filter;
}

void FolderModel::setFilter(const QString &filter)
{
    if (filter != d->filter) {
        d->filter = filter;
        refresh();
        emit filterChanged();
    }
}

bool FolderModel::isBusy() const
{
    return d->listJob != 0;
}

/** Refresh the contents of the current folder.
 */
void FolderModel::refresh()
{
    removeRows(0, rootItem->children.size());
    d->listJob = d->fs->list(d->rootUrl, d->filter);
    emit isBusyChanged();
}

QUrl FolderModel::rootUrl() const
{
    return d->rootUrl;
}

void FolderModel::setRootUrl(const QUrl &rootUrl)
{
    bool check;
    Q_UNUSED(check);

    if (rootUrl == d->rootUrl)
        return;

    d->rootUrl = rootUrl;
    d->fs = d->fileSystems.value(rootUrl.scheme());
    if (d->fs) {
        refresh();
    } else {
        if (!photoInitialised) {
            FileSystem::setNetworkAccessManagerFactory(new PhotoNetworkAccessManagerFactory);
            photoInitialised = true;
        }

        d->fs = FileSystem::create(d->rootUrl, this);
        if (d->fs) {
            check = connect(d->fs, SIGNAL(jobFinished(FileSystemJob*)),
                            this, SLOT(_q_jobFinished(FileSystemJob*)));
            Q_ASSERT(check);

            d->fileSystems.insert(d->rootUrl.scheme(), d->fs);
            d->fs->open(d->rootUrl);
        } else {
            removeRows(0, rootItem->children.size());
        }
    }
    emit rootUrlChanged(d->rootUrl);
}

bool FolderModel::removeRow(int row)
{
    if (row < 0 || row >= rootItem->children.size())
        return false;

    FolderModelItem *item = static_cast<FolderModelItem*>(rootItem->children.at(row));
    if (item->isDir())
        d->fs->rmdir(item->url());
    else
        d->fs->remove(item->url());
    return true;
}

bool FolderModel::showFiles() const
{
    return d->showFiles;
}

void FolderModel::setShowFiles(bool show)
{
    if (show != d->showFiles) {
        d->showFiles = show;
        emit showFilesChanged();
    }
}

void FolderModel::upload(const QString &filePath)
{
    QString base = d->rootUrl.toString();
    while (base.endsWith("/"))
        base.chop(1);

    d->queue->upload(d->fs, filePath, base + "/" + QFileInfo(filePath).fileName());
}

FolderQueueModel *FolderModel::uploads() const
{
    return d->queue;
}

/** When a command finishes, process its results.
 *
 * @param job
 */
void FolderModel::_q_jobFinished(FileSystemJob *job)
{
    Q_ASSERT(d->fs);

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
        if (job == d->listJob) {
            d->permissions = job->allowedOperations();
            emit permissionsChanged();

            removeRows(0, rootItem->children.size());
            foreach (const FileInfo& info, job->results()) {
                if (!d->showFiles && !info.isDir())
                    continue;

                FolderModelItem *item = new FolderModelItem(info);
                addItem(item, rootItem);
            }
            d->listJob = 0;
            emit isBusyChanged();
        }
        break;
    default:
        break;
    }
}

/** When a photo changes, emit notifications.
 */
void FolderModel::_q_photoChanged(const QUrl &url, FileSystem::ImageSize size)
{
    Q_UNUSED(size);
    foreach (ChatModelItem *ptr, rootItem->children) {
        FolderModelItem *item = static_cast<FolderModelItem*>(ptr);
        if (item->url() == url) {
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
}

class PhotoQueueItem : public ChatModelItem
{
public:
    PhotoQueueItem();

    FileInfo info;
    FileInfoList items;
    QString sourcePath;
    FileSystem *fileSystem;
    bool finished;
    bool isUpload;

    FileSystemJob *job;
    QFile *jobOutput;
    qint64 jobDoneBytes;
    qint64 jobTotalBytes;

    qint64 doneBytes;
    qint64 doneFiles;
    qint64 totalBytes;
    qint64 totalFiles;
};

PhotoQueueItem::PhotoQueueItem()
    : fileSystem(0)
    , finished(false)
    , isUpload(false)
    , job(0)
    , jobOutput(0)
    , jobDoneBytes(0)
    , jobTotalBytes(0)
    , doneBytes(0)
    , doneFiles(0)
    , totalBytes(0)
    , totalFiles(0)
{
}

FolderQueueModel::FolderQueueModel(QObject *parent)
    : ChatModel(parent)
    , m_resizer(0)
    , m_resizerThread(0)
    , m_downloadItem(0)
    , m_uploadDevice(0)
    , m_uploadItem(0)
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

FolderQueueModel::~FolderQueueModel()
{
    m_resizerThread->quit();
    m_resizerThread->wait();
    delete m_resizer;
    delete m_resizerThread;
}

void FolderQueueModel::cancel(int row)
{
    if (row < 0 || row > rootItem->children.size() - 1)
        return;

    PhotoQueueItem *item = static_cast<PhotoQueueItem*>(rootItem->children.at(row));
    if (item->job) {
        item->items.clear();
        item->job->abort();
    } else {
        removeRow(row);
    }
}

QVariant FolderQueueModel::data(const QModelIndex &index, int role) const
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
        return item->info.name();
    } else if (role == SpeedRole) {
        // FIXME: implement speed calculation
        return 0;
    } else if (role == DoneBytesRole) {
        return item->doneBytes + item->jobDoneBytes;
    } else if (role == DoneFilesRole) {
        return item->doneFiles;
    } else if (role == TotalBytesRole) {
        return item->totalBytes;
    } else if (role == TotalFilesRole) {
        return item->totalFiles;
    }
    return QVariant();
}

void FolderQueueModel::download(FileSystem *fileSystem, const FileInfo &info, const QString &filter)
{
    bool check;
    Q_UNUSED(check);

    PhotoQueueItem *item = new PhotoQueueItem;
    item->fileSystem = fileSystem;
    item->info = info;

    if (info.isDir()) {
        item->job = item->fileSystem->list(info.url(), filter);

        check = connect(item->job, SIGNAL(finished()),
                        this, SLOT(_q_listFinished()));
        Q_ASSERT(check);
    } else {
        item->items << info;
        item->totalBytes = info.size();
        item->totalFiles = 1;
    }

    addItem(item, rootItem);
    processQueue();
}

void FolderQueueModel::upload(FileSystem *fileSystem, const QString &filePath, const QUrl &url)
{
    PhotoQueueItem *item = new PhotoQueueItem;

    item->isUpload = true;
    item->info.setName(QFileInfo(filePath).fileName());
    item->info.setUrl(url);
    item->sourcePath = filePath;
    item->fileSystem = fileSystem;
    item->totalFiles = 1;
    addItem(item, rootItem);

    processQueue();
}

void FolderQueueModel::processQueue()
{
    bool check;
    Q_UNUSED(check);

    // process downloads
    if (!m_downloadItem) {
        foreach (ChatModelItem *ptr, rootItem->children) {
            PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
            if (!item->isUpload && !item->items.isEmpty()) {
                FileInfo childInfo = item->items.takeFirst();

                // determine path
                QString dirPath(wApp->settings()->sharesLocation());
                if (item->info.isDir()) {
                    const QString targetDir = item->info.name();
                    if (!targetDir.isEmpty()) {
                        QDir dir(dirPath);
                        if (dir.exists(targetDir) || dir.mkpath(targetDir))
                            dirPath = dir.filePath(targetDir);
                    }
                }
                const QString filePath = availableFilePath(dirPath, childInfo.name() + ".part");

                m_downloadItem = item;
                m_downloadItem->job = item->fileSystem->get(childInfo.url(), FileSystem::FullSize);
                m_downloadItem->jobOutput = new QFile(filePath, this);
                m_downloadItem->jobOutput->open(QIODevice::WriteOnly);
                m_downloadItem->jobTotalBytes = childInfo.size();

                check = connect(m_downloadItem->job, SIGNAL(finished()),
                                this, SLOT(_q_downloadFinished()));
                Q_ASSERT(check);

                check = connect(m_downloadItem->job, SIGNAL(readyRead()),
                                this, SLOT(_q_downloadReadyRead()));
                Q_ASSERT(check);

                check = connect(m_downloadItem->job, SIGNAL(downloadProgress(qint64,qint64)),
                                this, SLOT(_q_downloadProgress(qint64,qint64)));
                Q_ASSERT(check);
                break;
            }
        }
    }

    // process uploads
    if (!m_uploadItem) {
        foreach (ChatModelItem *ptr, rootItem->children) {
            PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
            if (item->isUpload && !item->finished) {
                m_uploadItem = item;
                QMetaObject::invokeMethod(m_resizer, "resize", Q_ARG(QString, item->sourcePath));
                break;
            }
        }
    }
}

/** When an download finishes, process the results.
 */
void FolderQueueModel::_q_downloadFinished()
{
    bool check;
    Q_UNUSED(check);

    if (m_downloadItem) {
        FileSystemJob *job = m_downloadItem->job;

        if (job->error() == FileSystemJob::NoError) {
            m_downloadItem->jobOutput->write(m_downloadItem->job->readAll());

            // rename file
            QFileInfo tempInfo(m_downloadItem->jobOutput->fileName());
            const QString finalPath = availableFilePath(tempInfo.dir().path(), tempInfo.fileName().remove(QRegExp("\\.part$")));
            m_downloadItem->jobOutput->rename(finalPath);
        } else {
            m_downloadItem->jobOutput->remove();
        }

        m_downloadItem->job->deleteLater();
        m_downloadItem->job = 0;
        m_downloadItem->doneBytes += m_downloadItem->jobTotalBytes;
        m_downloadItem->doneFiles++;
        m_downloadItem->jobDoneBytes = 0;

        if (m_downloadItem->items.isEmpty())
            removeItem(m_downloadItem);
        else
            emit dataChanged(createIndex(m_downloadItem), createIndex(m_downloadItem));

        m_downloadItem = 0;
    }

    processQueue();
}

/** When upload progress changes, emit notifications.
 */
void FolderQueueModel::_q_downloadProgress(qint64 done, qint64 total)
{
    if (m_downloadItem) {
        m_downloadItem->jobDoneBytes = done;
        emit dataChanged(createIndex(m_downloadItem), createIndex(m_downloadItem));
    }
}

void FolderQueueModel::_q_downloadReadyRead()
{
    Q_ASSERT(m_downloadItem);

    m_downloadItem->jobOutput->write(m_downloadItem->job->readAll());
}

void FolderQueueModel::_q_listFinished()
{
    FileSystemJob *job = qobject_cast<FileSystemJob*>(sender());
    if (!job)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoQueueItem *item = static_cast<PhotoQueueItem*>(ptr);
        if (item->job == job) {
            if (job->error() != FileSystemJob::NoError) {
                removeItem(item);
            } else {
                foreach (const FileInfo &child, job->results()) {
                    // FIXME: recurse
                    if (!child.isDir()) {
                        item->items << child;
                        item->totalFiles++;
                        item->totalBytes += child.size();
                    }
                }
                emit dataChanged(createIndex(item), createIndex(item));
                processQueue();
            }
            break;
        }
    }
}

/** When an upload finishes, process the results.
 */
void FolderQueueModel::_q_uploadFinished()
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
void FolderQueueModel::_q_uploadProgress(qint64 done, qint64 total)
{
    if (m_uploadItem) {
        m_uploadItem->doneBytes = done;
        m_uploadItem->totalBytes = total;
        emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
    }
}

void FolderQueueModel::_q_uploadResized(QIODevice *device)
{
    bool check;
    Q_UNUSED(check);

    if (m_uploadItem) {
        m_uploadDevice = device;
        m_uploadItem->job = m_uploadItem->fileSystem->put(m_uploadItem->info.url(), m_uploadDevice);
        check = connect(m_uploadItem->job, SIGNAL(finished()),
                        this, SLOT(_q_uploadFinished()));
        Q_ASSERT(check);

        check = connect(m_uploadItem->job, SIGNAL(uploadProgress(qint64,qint64)),
                        this, SLOT(_q_uploadProgress(qint64,qint64)));
        Q_ASSERT(check);
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

