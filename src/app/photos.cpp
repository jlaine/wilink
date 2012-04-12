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

#define BLOCK_SIZE 16384

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
        if (image->load(m_downloadJob, NULL)) {
            photoImageCache.insert(QString::number(m_downloadItem->type) + m_downloadItem->url.toString(), image);
            emit photoChanged(m_downloadItem->url, m_downloadItem->type);
        } else {
            qWarning("Could not load image %s", qPrintable(m_downloadItem->url.toString()));
            delete image;
        }
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

FolderIterator::FolderIterator(FileSystem *fileSystem, const FileInfo &info, const QString &filter, const QUrl &destination, QObject *parent)
    : QObject(parent)
    , m_filter(filter)
    , m_fs(fileSystem)
    , m_job(0)
{
    qRegisterMetaType<FileInfoList>("FileInfoList");
    if (info.isDir()) {
        m_queue.append(qMakePair(info.url(), FileInfo::fileUrl(destination, info.name())));
        processQueue();
    } else {
        FileInfoList results;
        results << info;
        QMetaObject::invokeMethod(this, "results", Qt::QueuedConnection, Q_ARG(FileInfoList, results), Q_ARG(QUrl, destination));
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
}

void FolderIterator::processQueue()
{
    bool check;
    Q_UNUSED(check);

    if (m_queue.isEmpty()) {
        emit finished();
        return;
    }

    m_current = m_queue.takeFirst();
    m_job = m_fs->list(m_current.first, m_filter);
    check = connect(m_job, SIGNAL(finished()),
                    this, SLOT(_q_listFinished()));
    Q_ASSERT(check);
}

void FolderIterator::abort()
{
    m_queue.clear();
    if (m_job)
        m_job->abort();
}

void FolderIterator::_q_listFinished()
{
    if (!m_job || sender() != m_job)
        return;

    if (m_job->error() == FileSystemJob::NoError) {
        emit results(m_job->results(), m_current.second);
        foreach (const FileInfo &child, m_job->results()) {
            if (child.isDir()) {
                m_queue.append(qMakePair(child.url(), FileInfo::fileUrl(m_current.second, child.name())));
            }
        }
    }

    m_job->deleteLater();
    m_job = 0;
    processQueue();
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
    QMap<QString,FileSystem*> fileSystems;
    FileSystemJob *listJob;
    FileSystemJob *openJob;
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
    , openJob(0)
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
    bool check;
    Q_UNUSED(check);

    d->listJob = d->fs->list(d->rootUrl, d->filter);
    check = connect(d->listJob, SIGNAL(finished()),
                    this, SLOT(_q_listFinished()));
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
            // register filesystem
            d->fileSystems.insert(d->rootUrl.scheme(), d->fs);
            check = connect(d->fs, SIGNAL(directoryChanged(QUrl)),
                            this, SLOT(_q_directoryChanged(QUrl)));
            Q_ASSERT(check);

            // open file system
            d->openJob = d->fs->open(d->rootUrl);
            check = connect(d->openJob, SIGNAL(finished()),
                            this, SLOT(_q_openFinished()));
            Q_ASSERT(check);
        } else {
            removeRows(0, rootItem->children.size());
        }
    }
    emit rootUrlChanged(d->rootUrl);
}

bool FolderModel::remove(int row)
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
void FolderModel::_q_openFinished()
{
    if (sender() != d->openJob)
        return;

    if (d->openJob->error() == FileSystemJob::NoError) {
        refresh();
    }

    d->openJob->deleteLater();
    d->openJob = 0;
}

void FolderModel::_q_listFinished()
{
    if (sender() != d->listJob)
        return;

    if (d->listJob->error() == FileSystemJob::NoError) {
        d->permissions = d->listJob->allowedOperations();
        emit permissionsChanged();

        removeRows(0, rootItem->children.size());
        foreach (const FileInfo& info, d->listJob->results())
            addItem(new FolderModelItem(info), rootItem);

#if 0
        // collect old items
        QMultiMap<QUrl,FolderModelItem*> remaining;
        foreach (ChatModelItem *ptr, rootItem->children) {
            FolderModelItem *item = static_cast<FolderModelItem*>(ptr);
            remaining.insert(item->url(), item);
        }

        // create or update items
        int newRow = 0;
        foreach (const FileInfo& info, d->listJob->results()) {
            if (!d->showFiles && !info.isDir())
                continue;

            FolderModelItem *item = remaining.take(info.url());
            if (item) {
                const int oldRow = item->row();
                if (oldRow != newRow) {
                    beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
                    rootItem->children.move(oldRow, newRow);
                    endMoveRows();
                }

                *item = info;
                emit dataChanged(createIndex(item), createIndex(item));
            } else {
                item = new FolderModelItem(info);
                addItem(item, rootItem, newRow);
            }
            newRow++;
        }

        // remove obsolete items
        foreach (FolderModelItem *item, remaining.values())
            removeItem(item);
#endif
    } else {
        removeRows(0, rootItem->children.size());
    }

    d->listJob->deleteLater();
    d->listJob = 0;
    emit isBusyChanged();
}

/** When a directory changes, refresh listing.
 */
void FolderModel::_q_directoryChanged(const QUrl &url)
{
    if (url == d->rootUrl)
        refresh();
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

class FolderQueueFile
{
public:
    FolderQueueFile(const FileInfo &info, const QUrl &destination)
        : fileInfo(info)
        , destinationUrl(destination)
    {
    }

    FileInfo fileInfo;
    QUrl destinationUrl;
};

class FolderQueueItem : public ChatModelItem
{
public:
    FolderQueueItem();

    FileInfo info;
    QList<FolderQueueFile> items;
    FolderIterator *iterator;
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

FolderQueueItem::FolderQueueItem()
    : fileSystem(0)
    , finished(false)
    , iterator(0)
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

    FolderQueueItem *item = static_cast<FolderQueueItem*>(rootItem->children.at(row));
    if (item->job) {
        item->items.clear();
        item->job->abort();
    } else {
        removeRow(row);
    }
}

QVariant FolderQueueModel::data(const QModelIndex &index, int role) const
{
    FolderQueueItem *item = static_cast<FolderQueueItem*>(index.internalPointer());
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

    FolderQueueItem *item = new FolderQueueItem;
    item->fileSystem = fileSystem;
    item->info = info;

    const QUrl destUrl = QUrl::fromLocalFile(wApp->settings()->sharesLocation());

    item->iterator = new FolderIterator(fileSystem, info, filter, destUrl);

    check = connect(item->iterator, SIGNAL(finished()),
                    this, SLOT(_q_iteratorFinished()));
    Q_ASSERT(check);

    check = connect(item->iterator, SIGNAL(results(FileInfoList,QUrl)),
                    this, SLOT(_q_iteratorResults(FileInfoList,QUrl)));
    Q_ASSERT(check);

    addItem(item, rootItem);
}

void FolderQueueModel::upload(FileSystem *fileSystem, const QString &filePath, const QUrl &url)
{
    FolderQueueItem *item = new FolderQueueItem;

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
            FolderQueueItem *item = static_cast<FolderQueueItem*>(ptr);
            if (!item->isUpload && !item->items.isEmpty()) {
                FolderQueueFile xfer = item->items.takeFirst();

                // determine path
                QDir dir(xfer.destinationUrl.toLocalFile());
                if (!dir.exists()) {
                    QDir parentDir(QFileInfo(dir.path()).dir().path());
                    if (!parentDir.mkdir(dir.dirName())) {
                        qWarning("Could not create %s directory", qPrintable(dir.path()));
                        continue;
                    }
                }
                const QString filePath = availableFilePath(dir.path(), xfer.fileInfo.name() + ".part");

                // open output
                QFile *output = new QFile(filePath, this);
                if (!output->open(QIODevice::WriteOnly)) {
                    qWarning("Could not open %s for writing", qPrintable(filePath));
                    delete output;
                    continue;
                }

                m_downloadItem = item;
                m_downloadItem->job = item->fileSystem->get(xfer.fileInfo.url(), FileSystem::FullSize);
                m_downloadItem->jobOutput = output;
                m_downloadItem->jobTotalBytes = xfer.fileInfo.size();

                check = connect(m_downloadItem->job, SIGNAL(finished()),
                                this, SLOT(_q_downloadFinished()));
                Q_ASSERT(check);

                check = connect(m_downloadItem->job, SIGNAL(readyRead()),
                                this, SLOT(_q_downloadData()));
                Q_ASSERT(check);

                check = connect(m_downloadItem->job, SIGNAL(downloadProgress(qint64,qint64)),
                                this, SLOT(_q_downloadProgress(qint64,qint64)));
                Q_ASSERT(check);

                check = connect(m_downloadItem->jobOutput, SIGNAL(bytesWritten(qint64)),
                                this, SLOT(_q_downloadData()));
                Q_ASSERT(check);
                break;
            }
        }
    }

    // process uploads
    if (!m_uploadItem) {
        foreach (ChatModelItem *ptr, rootItem->children) {
            FolderQueueItem *item = static_cast<FolderQueueItem*>(ptr);
            if (item->isUpload && !item->finished) {
                m_uploadItem = item;
                QMetaObject::invokeMethod(m_resizer, "resize", Q_ARG(QString, item->sourcePath));
                break;
            }
        }
    }
}

/** Transfer the next block of data.
 */
void FolderQueueModel::_q_downloadData()
{
    Q_ASSERT(m_downloadItem);

    // don't saturate output
    if (m_downloadItem->jobOutput->bytesToWrite() > 2 * BLOCK_SIZE)
        return;

    char buffer[BLOCK_SIZE];
    const qint64 length = m_downloadItem->job->read(buffer, BLOCK_SIZE);
    if (length < 0) {
        qWarning("Download could not read from input, aborting");
        m_downloadItem->job->abort();
    } else if (length > 0) {
        m_downloadItem->jobOutput->write(buffer, length);
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

void FolderQueueModel::_q_iteratorFinished()
{
    FolderIterator *iterator = qobject_cast<FolderIterator*>(sender());
    if (!iterator)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        FolderQueueItem *item = static_cast<FolderQueueItem*>(ptr);
        if (item->iterator == iterator) {
            item->iterator->deleteLater();
            item->iterator = 0;
            if (item->items.isEmpty()) {
                removeItem(item);
            } else {
                emit dataChanged(createIndex(item), createIndex(item));
                processQueue();
            }
            break;
        }
    }
}

void FolderQueueModel::_q_iteratorResults(const FileInfoList &results, const QUrl &dest)
{
    FolderIterator *iterator = qobject_cast<FolderIterator*>(sender());
    if (!iterator)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        FolderQueueItem *item = static_cast<FolderQueueItem*>(ptr);
        if (item->iterator == iterator) {
            foreach (const FileInfo &child, results) {
                if (!child.isDir()) {
                    item->items << FolderQueueFile(child, dest);
                    item->totalFiles++;
                    item->totalBytes += child.size();
                }
            }
            emit dataChanged(createIndex(item), createIndex(item));
            break;
        }
    }
}


/** When upload progress changes, emit notifications.
 */
void FolderQueueModel::_q_downloadProgress(qint64 done, qint64 total)
{
    Q_UNUSED(total);

    if (m_downloadItem) {
        m_downloadItem->jobDoneBytes = done;
        emit dataChanged(createIndex(m_downloadItem), createIndex(m_downloadItem));
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

