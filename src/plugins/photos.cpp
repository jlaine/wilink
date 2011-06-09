/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include "QXmppClient.h"

#include "window.h"
#include "photos.h"

static const QSize UPLOAD_SIZE(2048, 2048);

static PhotoCache *photoCache = 0;
static QCache<QUrl, QImage> photoImageCache;

enum PhotoRole
{
    IsDirRole = ChatModel::UserRole,
    ProgressRole,
    SizeRole,
};

static bool isImage(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    QString extension = fileName.split(".").last().toLower();
    return (extension == "gif" || extension == "jpg" || extension == "jpeg" || extension == "png");
}

PhotoCache::PhotoCache()
    : m_downloadDevice(0)
{
}

void PhotoCache::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    Q_UNUSED(results);
    if (cmd != FileSystem::Get)
        return;

    Q_ASSERT(m_downloadDevice);

    if (error) {
        m_downloadDevice->deleteLater();
        m_downloadDevice = 0;
    } else {
        // load image
        m_downloadDevice->reset();
        QImage *image = new QImage;
        image->load(m_downloadDevice, NULL);
        m_downloadDevice->close();
        m_downloadDevice->deleteLater();
        m_downloadDevice = 0;

        photoImageCache.insert(m_downloadUrl, image);
        emit photoChanged(m_downloadUrl);
    }

    processQueue();
}

QUrl PhotoCache::imageUrl(const QUrl &url, FileSystem *fs)
{
    if (photoImageCache.contains(url)) {
        QUrl cacheUrl("image://photo");
        cacheUrl.setPath("/" + url.toString());
        return cacheUrl;
    }

    // check if the url is already queued
    bool found = false;
    QPair<QUrl, FileSystem*> job;
    foreach (job, m_downloadQueue) {
        if (job.first == url) {
            found = true;
            break;
        }
    }
    if (!found) {
        m_downloadQueue.append(qMakePair(url, fs));
        processQueue();
    }

    return QUrl(":/file-128.png");
}

PhotoCache *PhotoCache::instance()
{
    if (!photoCache)
        photoCache = new PhotoCache;
    return photoCache;
}

void PhotoCache::processQueue()
{
    if (m_downloadDevice || m_downloadQueue.isEmpty())
        return;

    QPair<QUrl, FileSystem*> job = m_downloadQueue.takeFirst();
    FileSystem *fs = job.second;
    if (!m_fileSystems.contains(fs)) {
        m_fileSystems << fs;
        connect(fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)),
                this, SLOT(commandFinished(int, bool, const FileInfoList&)));
    }
    m_downloadUrl = job.first;
    m_downloadDevice = fs->get(m_downloadUrl, FileSystem::MediumSize);
}

PhotoImageProvider::PhotoImageProvider()
    : QDeclarativeImageProvider(Image)
{
}

QImage PhotoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;
    QImage *cached = photoImageCache.object(QUrl(id));
    if (cached) {
        image = *cached;
    } else {
        qWarning("Could not get photo for %s", qPrintable(id));
        image = QImage(":/file-128.png");
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
    m_fs(0)
{
    QHash<int, QByteArray> names = roleNames();
    names.insert(IsDirRole, "isDir");
    names.insert(SizeRole, "size");
    setRoleNames(names);

    m_uploads = new PhotoUploadModel(this);

    connect(PhotoCache::instance(), SIGNAL(photoChanged(QUrl)),
            this, SLOT(photoChanged(QUrl)));
}

/** When a command finishes, process its results.
 *
 * @param cmd
 * @param error
 * @param results
 */
void PhotoModel::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    Q_ASSERT(m_fs);

    if (error) {
        qWarning() << m_fs->commandName(cmd) << "command failed";
        return;
    }

    switch (cmd)
    {
    case FileSystem::Open:
    case FileSystem::Mkdir:
    case FileSystem::Put:
    case FileSystem::Remove:
        refresh();
        break;
    case FileSystem::List:
        removeRows(0, rootItem->children.size());
        foreach (const FileInfo& info, results) {
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

QVariant PhotoModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(m_fs);
    PhotoItem *item = static_cast<PhotoItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        if (item->isDir()) {
            return QUrl("qrc:/album-128.png");
        } else {
            return PhotoCache::instance()->imageUrl(item->url(), m_fs);
        }
    } else if (role == IsDirRole)
        return item->isDir();
    else if (role == NameRole)
        return item->name();
    else if (role == SizeRole)
        return item->size();
    else if (role == UrlRole)
        return item->url();
    return QVariant();
}

void PhotoModel::createAlbum(const QString &name)
{
    if (name.isEmpty())
        return;

    QUrl newUrl = m_rootUrl;
    newUrl.setPath(newUrl.path() + "/" + name);
    m_fs->mkdir(newUrl);
}

void PhotoModel::photoChanged(const QUrl &url)
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoItem *item = static_cast<PhotoItem*>(ptr);
        if (item->url() == url)
            emit dataChanged(createIndex(item), createIndex(item));
    }
}

/** Refresh the contents of the current folder.
 */
void PhotoModel::refresh()
{
    if (m_fs)
        m_fs->list(m_rootUrl);
}

QUrl PhotoModel::rootUrl() const
{
    return m_rootUrl;
}

void PhotoModel::setRootUrl(const QUrl &rootUrl)
{
    if (rootUrl == m_rootUrl)
        return;

    m_rootUrl = rootUrl;

    if (!m_fs) {
        bool check;

        m_fs = FileSystem::factory(m_rootUrl, this);
        check = connect(m_fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)),
                        this, SLOT(commandFinished(int, bool, const FileInfoList&)));
        Q_ASSERT(check);

        check = connect(m_fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)),
                        m_uploads, SLOT(commandFinished(int, bool, const FileInfoList&)));
        Q_ASSERT(check);

        check = connect(m_fs, SIGNAL(putProgress(int, int)),
                        m_uploads, SLOT(putProgress(int, int)));
        Q_ASSERT(check);

        m_fs->open(m_rootUrl);
    } else {
        m_fs->list(m_rootUrl);
    }

    emit rootUrlChanged(m_rootUrl);
}

void PhotoModel::upload(const QString &filePath)
{
    QString base = m_rootUrl.toString();
    while (base.endsWith("/"))
        base.chop(1);

    m_uploads->append(filePath, m_fs, base + "/" + QFileInfo(filePath).fileName());
}

PhotoUploadModel *PhotoModel::uploads() const
{
    return m_uploads;
}

class PhotoUploadItem : public ChatModelItem
{
public:
    QString sourcePath;
    QString destinationPath;
    FileSystem *fileSystem;
    bool finished;
    qreal progress;
    qint64 size;
};

PhotoUploadModel::PhotoUploadModel(QObject *parent)
    : ChatModel(parent),
    m_uploadDevice(0),
    m_uploadItem(0)
{
    QHash<int, QByteArray> names = roleNames();
    names.insert(ProgressRole, "progress");
    names.insert(SizeRole, "size");
    setRoleNames(names);
}

#if 0
void PhotoUploadModel::abort()
{
    //fs->abort();
}
#endif

void PhotoUploadModel::append(const QString &filePath, FileSystem *fileSystem, const QString &destinationPath)
{
    qDebug("Adding item %s", qPrintable(filePath));
    PhotoUploadItem *item = new PhotoUploadItem;
    item->sourcePath = filePath;
    item->destinationPath = destinationPath;
    item->fileSystem = fileSystem;
    addItem(item, rootItem);

    processQueue();
}

void PhotoUploadModel::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    Q_UNUSED(error);
    Q_UNUSED(results);

    if (cmd != FileSystem::Put)
        return;

    if (m_uploadItem) {
        m_uploadItem->finished = true;
        emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
        m_uploadItem = 0;

        delete m_uploadDevice;
        m_uploadDevice = 0;
    }

    processQueue();
}

QVariant PhotoUploadModel::data(const QModelIndex &index, int role) const
{
    PhotoUploadItem *item = static_cast<PhotoUploadItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        return QUrl::fromLocalFile(item->sourcePath);
    } else if (role == NameRole) {
        return QFileInfo(item->sourcePath).fileName();
    } else if (role == ProgressRole) {
        return item->progress;
    } else if (role == SizeRole) {
        return item->size;
    }
    return QVariant();
}

void PhotoUploadModel::processQueue()
{
    if (m_uploadItem)
        return;

    foreach (ChatModelItem *ptr, rootItem->children) {
        PhotoUploadItem *item = static_cast<PhotoUploadItem*>(ptr);
        if (!item->finished) {
            m_uploadItem = item;

            // process the next file to upload
            const QByteArray imageFormat = QImageReader::imageFormat(item->sourcePath);
            QImage image;
            if (!imageFormat.isEmpty() && image.load(item->sourcePath, imageFormat.constData()))
            {
                if (image.width() > UPLOAD_SIZE.width() || image.height() > UPLOAD_SIZE.height())
                    image = image.scaled(UPLOAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_uploadDevice = new QBuffer(this);
                m_uploadDevice->open(QIODevice::WriteOnly);
                image.save(m_uploadDevice, imageFormat.constData());
                m_uploadDevice->open(QIODevice::ReadOnly);
            } else {
                m_uploadDevice = new QFile(item->sourcePath, this);
            }

            item->fileSystem->put(m_uploadDevice, item->destinationPath);
            break;
        }
    }
}

/** TODO: Update the progress bar for the current upload.
 */
void PhotoUploadModel::putProgress(int done, int total)
{
    if (m_uploadItem) {
        m_uploadItem->size = total;
        m_uploadItem->progress = total ? qreal(done) / qreal(total) : 0;
        emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
    }
}

