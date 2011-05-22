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
#include <QDebug>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeImageProvider>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QUrl>

#include "QXmppClient.h"

#include "chat.h"
#include "chat_plugin.h"
#include "photos.h"

static const int PROGRESS_STEPS = 100;
static const QSize ICON_SIZE(128, 128);
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

PhotosList::PhotosList(const QUrl &url, QWidget *parent)
    : QListWidget(parent), baseDrop(true), baseUrl(url)
{
    setIconSize(ICON_SIZE);
    setSpacing(5);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setSortingEnabled(true);
    setTextElideMode(Qt::ElideMiddle);
    setWordWrap(true);
    setWrapping(true);
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(slotItemDoubleClicked(QListWidgetItem *)));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(slotReturnPressed()));
}

void PhotosList::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        QListWidgetItem *dropItem = itemAt(event->pos());
        if (dropItem)
            setCurrentItem(dropItem);
    } else {
        event->ignore();
    }
}

void PhotosList::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        QListWidgetItem *dropItem = itemAt(event->pos());
        if (dropItem)
            setCurrentItem(dropItem);
    } else {
        event->ignore();
    }
}

void PhotosList::dropEvent(QDropEvent *event)
{
    const FileInfo &info = itemEntry(itemAt(event->pos()));
    if (!event->mimeData()->hasUrls() || (!info.isDir() && !baseDrop))
    {
        event->ignore();
        return;
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();

    emit filesDropped(event->mimeData()->urls(),
        info.isDir() ? info.url() : baseUrl);
}

FileInfo PhotosList::itemEntry(QListWidgetItem *item) const
{
    if (!item)
        return FileInfo();

    QUrl url = item->data(Qt::UserRole).value<QUrl>();
    foreach (const FileInfo &info, fileList)
    {
        if (info.url() == url)
            return info;
    }
    return FileInfo();
}

void PhotosList::setBaseDrop(bool accept)
{
    baseDrop = accept;
}

FileInfoList PhotosList::entries() const
{
    return fileList;
}

void PhotosList::setEntries(const FileInfoList &entries)
{
    fileList = entries;
    clear();
    foreach (const FileInfo& info, fileList)
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        if (info.isDir()) {
            newItem->setIcon(QIcon(":/album-128.png"));
        } else {
            newItem->setIcon(QIcon(":/file-128.png"));
        }
        newItem->setData(Qt::UserRole, QUrl(info.url()));
        newItem->setText(info.name());
        addItem(newItem);
    }
    if (count())
        setCurrentRow(0);
}

void PhotosList::setImage(const QUrl &url, const QImage &img)
{
    for (int i = 0; i < count(); ++i)
    {
        QListWidgetItem *currentItem = item(i);
        if (currentItem->data(Qt::UserRole).value<QUrl>() == url)
        {
            QPixmap pixmap(ICON_SIZE);
            const QImage &scaled = img.scaled(pixmap.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pixmap.fill();
            QPainter painter(&pixmap);
            painter.drawImage((pixmap.width() - scaled.width()) / 2,
                (pixmap.height() - scaled.height()) / 2, scaled);
            currentItem->setIcon(pixmap);
        }
    }
}

FileInfoList PhotosList::selectedEntries() const
{
    FileInfoList entries;
    foreach (QListWidgetItem *item, selectedItems())
        entries << itemEntry(item);
    return entries;
}

void PhotosList::slotItemDoubleClicked(QListWidgetItem *item)
{
    const FileInfo &info = itemEntry(item);
    if (info.isDir())
        emit folderOpened(info.url());
    else
        emit fileOpened(info.url());
}

void PhotosList::slotReturnPressed()
{
    QListWidgetItem *item = currentItem();
    if (item)
        slotItemDoubleClicked(item);
}

QUrl PhotosList::url()
{
    return baseUrl;
}

PhotoCache::PhotoCache()
    : m_downloadDevice(0)
{
}

void PhotoCache::commandFinished(int cmd, bool error, const FileInfoList &results)
{
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

class PhotoImageProvider : public QDeclarativeImageProvider
{
public:
    PhotoImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

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

    if (error)
        qWarning() << m_fs->commandName(cmd) << "command failed";

    switch (cmd)
    {
    case FileSystem::Open:
        if (!error)
            refresh();
        break;
    case FileSystem::List: {
        if (error)
            return;

        removeRows(0, rootItem->children.size());
        foreach (const FileInfo& info, results) {
            if (info.isDir() || isImage(info.name())) {
                PhotoItem *item = new PhotoItem(info);
                addItem(item, rootItem);
            }
        }
        break;
    }
    case FileSystem::Mkdir:
        refresh();
        break;
    case FileSystem::Remove:
        if (!error)
            refresh();
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
        connect(m_fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)),
                this, SLOT(commandFinished(int, bool, const FileInfoList&)));
        connect(m_fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)),
                m_uploads, SLOT(commandFinished(int, bool, const FileInfoList&)));
        connect(m_fs, SIGNAL(putProgress(int, int)),
                m_uploads, SLOT(putProgress(int, int)));

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
    if (cmd != FileSystem::Put)
        return;

    m_uploadItem->finished = true;
    emit dataChanged(createIndex(m_uploadItem), createIndex(m_uploadItem));
    m_uploadItem = 0;

    delete m_uploadDevice;
    m_uploadDevice = 0;
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

/** Constructs a PhotoPanel.
 *
 * @param url    The base URL for the photo sharing service.
 * @param parent The parent widget of the panel.
 */
PhotoPanel::PhotoPanel(Chat *chatWindow, const QString &url)
    : ChatPanel(chatWindow),
    baseUrl(url),
    downloadDevice(0),
    uploadDevice(0),
    progressFiles(0)
{
    bool check;

    /* create UI */
    setWindowHelp(tr("To upload your photos to wifirst.net, simply drag and drop them to an album."));
    setWindowTitle(tr("Photos"));

    photosView = new QStackedWidget;
    PhotosList *listView = new PhotosList(url);
    listView->setBaseDrop(false);
    photosView->addWidget(listView);
    connect(listView, SIGNAL(fileOpened(const QUrl&)),
            this, SLOT(fileOpened(const QUrl&)));
    connect(listView, SIGNAL(filesDropped(const QList<QUrl>&, const QUrl&)),
            this, SLOT(filesDropped(const QList<QUrl>&, const QUrl&)));
    connect(listView, SIGNAL(folderOpened(const QUrl&)),
            this, SLOT(folderOpened(const QUrl&)));

    progressBar = new QProgressBar;
    progressBar->setRange(0, 0);
    progressBar->setValue(0);
    progressBar->hide();

    statusLabel = new QLabel;

    backAction = addAction(QIcon(":/back.png"), tr("Go back"));
    backAction->setEnabled(false);
    backAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    connect(backAction, SIGNAL(triggered()), this, SLOT(goBack()));

    stopButton = new QPushButton(tr("Cancel"));
    stopButton->setIcon(QIcon(":/close.png"));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(abortUpload()));
    stopButton->hide();

    createAction = addAction(QIcon(":/add.png"), tr("Create an album"));
    connect(createAction, SIGNAL(triggered()), this, SLOT(createFolder()));

    deleteAction = addAction(QIcon(":/remove.png"), tr("Delete"));
#ifdef Q_OS_MAC
    deleteAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Backspace));
#else
    deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
#endif
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteFile()));
    deleteAction->setVisible(false);

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addLayout(headerLayout());
    //layout->addWidget(photosView);
    QHBoxLayout *hbox_upload = new QHBoxLayout;
    hbox_upload->addWidget(stopButton);
    hbox_upload->addWidget(progressBar);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(statusLabel);
    hbox->addStretch();
    layout->addLayout(hbox_upload);
    layout->addLayout(hbox);

    // declarative
    QDeclarativeView *declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("window", chatWindow);
    context->setContextProperty("baseUrl", QVariant::fromValue(url));

    declarativeView->engine()->addImageProvider("photo", new PhotoImageProvider);
    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/PhotoPanel.qml"));
    layout->addWidget(declarativeView);

    // connect signals
    check = connect(declarativeView->rootObject(), SIGNAL(close()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    /* open filesystem */
    fs = FileSystem::factory(url, this);
    connect(fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)), this,
            SLOT(commandFinished(int, bool, const FileInfoList&)));
    connect(fs, SIGNAL(putProgress(int, int)), this, SLOT(putProgress(int, int)));

    setFocusProxy(photosView);
}

/** When a command finishes, process its results.
 *
 * @param cmd
 * @param error
 * @param results
 */
void PhotoPanel::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    if (error)
        qWarning() << fs->commandName(cmd) << "command failed";

    switch (cmd)
    {
    case FileSystem::Get:
        if (!error && photosView->indexOf(downloadJob.widget) >= 0)
        {
            /* load image */
            downloadDevice->reset();
            QImage img;
            img.load(downloadDevice, NULL);
            downloadDevice->close();

            /* display image */
            PhotosList *listView = qobject_cast<PhotosList *>(downloadJob.widget);
            QLabel *label = qobject_cast<QLabel *>(downloadJob.widget);
            if (listView)
                listView->setImage(downloadJob.remoteUrl, img);
            else if (label)
            {
                QSize maxSize = photosView->size();
                if (img.width() > maxSize.width() || img.height() > maxSize.height())
                    img = img.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                label->setPixmap(QPixmap::fromImage(img));
            }
        }

        /* fetch next thumbnail */
        downloadJob.clear();
        processDownloadQueue();
        break;
    case FileSystem::Open:
        if (!error)
            refresh();
        break;
    case FileSystem::List: {
        if (error)
            return;

        /* show entries */
        PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
        Q_ASSERT(listView != NULL);
        listView->setEntries(results);

        /* drag and drop is now allowed */
        showMessage();
        listView->setAcceptDrops(true);
        if (photosView->count() > 1)
        {
            backAction->setEnabled(true);
            createAction->setVisible(false);
            deleteAction->setVisible(true);
        }

        /* fetch thumbnails */
        foreach (const FileInfo& info, results)
        {
            if (!info.isDir() && isImage(info.name()))
                downloadQueue.append(Job(listView, info.url(), FileSystem::SmallSize));
        }
        processDownloadQueue();
        break;
    }
    case FileSystem::Mkdir:
        refresh();
        break;
    case FileSystem::Put:
        progressFiles++;
        progressBar->setValue(PROGRESS_STEPS * progressFiles);
        delete uploadDevice;
        uploadDevice = 0;
        processUploadQueue();
        break;
    case FileSystem::Remove:
        if (!error)
            refresh();
        break;
    default:
        qWarning() << fs->commandName(cmd) << "was not expected";
        break;
    }
}

void PhotoPanel::abortUpload()
{
    showMessage(tr("Cancelling upload.."));
    uploadQueue.clear();
    fs->abort();
}

/** Prompt the user for a new folder name and create it.
 */
void PhotoPanel::createFolder()
{
     bool ok;
     QString text = QInputDialog::getText(this, tr("Create an album"),
                                          tr("Album name:"), QLineEdit::Normal,
                                          "", &ok);
    if (ok && !text.isEmpty())
    {
        PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
        Q_ASSERT(listView != NULL);
        listView->setAcceptDrops(false);
        fs->mkdir(listView->url().toString() + "/" + text);
    }
}

void PhotoPanel::deleteFile()
{
    PhotosList *listView = qobject_cast<PhotosList*>(photosView->currentWidget());
    if (!listView)
        return;

    FileInfoList entries = listView->selectedEntries();
    foreach (const FileInfo &entry, entries)
        if (!entry.isDir())
        {
            showMessage(tr("Deleting %1").arg(entry.name()));
            fs->remove(entry.url());
        }
}

void PhotoPanel::fileNext()
{
    QLabel *label = qobject_cast<QLabel*>(photosView->currentWidget());
    if (!label)
        return;

    // check if we have reached end of playlist
    if (playList.isEmpty())
    {
        goBack();
        return;
    }

    // download image
    downloadQueue.append(Job(label, playList.takeFirst(), FileSystem::LargeSize));
    processDownloadQueue();
}

/** Display the file at the given URL.
 *
 * @param url
 */
void PhotoPanel::fileOpened(const QUrl &url)
{
    if (!isImage(url.toString()))
        return;

    // disable controls
    deleteAction->setVisible(false);

    // build playlist
    PhotosList *listView = qobject_cast<PhotosList*>(photosView->currentWidget());
    playList.clear();
    foreach (const FileInfo &info, listView->entries())
        playList << info.url();
    qSort(playList);
    int playListPosition = playList.indexOf(url);
    playList = playList.mid(playListPosition + 1) + playList.mid(0, playListPosition);

    // create white label
    QLabel *label = new QLabel(tr("Loading image.."));
    label->setAlignment(Qt::AlignCenter);
    label->setAutoFillBackground(true);
    QPalette pal = label->palette();
    pal.setColor(QPalette::Background, Qt::white);
    label->setPalette(pal);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Return), label);
    connect(shortcut, SIGNAL(activated()), this, SLOT(fileNext()));
    shortcut = new QShortcut(QKeySequence(Qt::Key_Enter), label);
    connect(shortcut, SIGNAL(activated()), this, SLOT(fileNext()));
    pushView(label);

    // download image
    downloadQueue.append(Job(label, url, FileSystem::LargeSize));
    processDownloadQueue();
}

/** Upload the given files to a remote folder.
 *
 * @param files
 * @param destination
 */
void PhotoPanel::filesDropped(const QList<QUrl> &files, const QUrl &destination)
{
    QString base = destination.toString();
    while (base.endsWith("/"))
        base.chop(1);

    foreach (const QUrl &url, files)
    {
        QFileInfo file(url.path());
        uploadQueue.append(QPair<QUrl, QUrl>(url, base + "/" + file.fileName()));
    }
    progressBar->setMaximum(progressBar->maximum() + PROGRESS_STEPS * files.size());
    progressBar->show();
    stopButton->show();
    processUploadQueue();
}

/** Open the remote folder at the given URL.
 *
 * @param url
 */
void PhotoPanel::folderOpened(const QUrl &url)
{
    PhotosList *listView = new PhotosList(url);
    pushView(listView);
    listView->setFocus();
    connect(listView, SIGNAL(fileOpened(const QUrl&)),
            this, SLOT(fileOpened(const QUrl&)));
    connect(listView, SIGNAL(filesDropped(const QList<QUrl>&, const QUrl&)),
            this, SLOT(filesDropped(const QList<QUrl>&, const QUrl&)));
    connect(listView, SIGNAL(folderOpened(const QUrl&)),
        this, SLOT(folderOpened(const QUrl&)));
    fs->list(url.toString());
}

/** Go back to the previous folder.
 */
void PhotoPanel::goBack()
{
    if (photosView->count() < 2 || !backAction->isEnabled())
        return;

    // remove obsolete items from download queue
    QWidget *goner = photosView->currentWidget();
    for (int i = downloadQueue.size() - 1; i >= 0; i--)
        if (downloadQueue.at(i).widget == goner)
            downloadQueue.removeAt(i);

    photosView->removeWidget(goner);
    photosView->currentWidget()->setFocus();

    /* enable controls */
    if (photosView->count() == 1)
    {
        backAction->setEnabled(false);
        deleteAction->setVisible(false);
        createAction->setVisible(true);
    } else {
        deleteAction->setVisible(true);
    }
}

/** Open filesystem.
 */
void PhotoPanel::open()
{
    showMessage(tr("Connecting.."));
    fs->open(baseUrl);
}

/** If the download queue is not empty, process the next item.
 */
void PhotoPanel::processDownloadQueue()
{
    if (downloadQueue.empty() || !downloadJob.isEmpty())
        return;

    downloadJob = downloadQueue.takeFirst();
    downloadDevice = fs->get(downloadJob.remoteUrl, downloadJob.type);
}

/** If the upload queue is not empty, process the next item.
 */
void PhotoPanel::processUploadQueue()
{
    if (uploadDevice)
        return;

    /* if the queue is empty, hide progress bar and reset it */
    if (uploadQueue.empty())
    {
        /* reset progress bar */
        stopButton->hide();
        progressBar->hide();
        progressBar->setValue(0);
        progressBar->setMaximum(0);
        progressFiles = 0;

        /* refresh the current view */
        refresh();

        /* notify the user */
        showMessage(tr("Photos upload complete."));
        queueNotification(tr("Your photos have been uploaded."), ForceNotification);
        return;
    }

    /* process the next file to upload */
    QPair<QUrl, QUrl> item = uploadQueue.takeFirst();
    const QString filePath = item.first.toLocalFile();
    const QByteArray imageFormat = QImageReader::imageFormat(filePath);
    QImage image;
    if (!imageFormat.isEmpty() && image.load(filePath, imageFormat.constData()))
    {
        if (image.width() > UPLOAD_SIZE.width() || image.height() > UPLOAD_SIZE.height())
            image = image.scaled(UPLOAD_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        uploadDevice = new QBuffer(this);
        uploadDevice->open(QIODevice::WriteOnly);
        image.save(uploadDevice, imageFormat.constData());
        uploadDevice->open(QIODevice::ReadOnly);
    } else {
        uploadDevice = new QFile(filePath, this);
    }

    showMessage(tr("Uploading %1").arg(QFileInfo(filePath).fileName()));
    fs->put(uploadDevice, item.second.toString());
}

void PhotoPanel::pushView(QWidget *widget)
{
    static int counter = 0;
    widget->setObjectName(QString::number(counter++));
    photosView->setCurrentIndex(photosView->addWidget(widget));
}

/** Update the progress bar for the current upload.
 */
void PhotoPanel::putProgress(int done, int total)
{
    if (!total)
        return;
    progressBar->setValue(PROGRESS_STEPS * (progressFiles + double(done) / double(total)));
}

/** Refresh the contents of the current folder.
 */
void PhotoPanel::refresh()
{
    PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
    if (!listView)
        return;

    showMessage(tr("Loading your albums.."));
    listView->setAcceptDrops(false);
    fs->list(listView->url());
}

void PhotoPanel::showMessage(const QString &message)
{
    if (message.isEmpty())
        statusLabel->hide();
    else {
        statusLabel->setText(message);
        statusLabel->show();
    }
}

// PLUGIN

class PhotosPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Photos"; };
};

bool PhotosPlugin::initialize(Chat *chat)
{
    qmlRegisterUncreatableType<PhotoUploadModel>("wiLink", 1, 2, "PhotoUploadModel", "");
    qmlRegisterType<PhotoModel>("wiLink", 1, 2, "PhotoModel");

    QString url;
    QString domain = chat->client()->configuration().domain();
    if (domain == "wifirst.net")
        url = "wifirst://www.wifirst.net/w";
    else if (domain == "gmail.com")
        url = "picasa://default";
    else
        return false;

    // register panel
    PhotoPanel *photos = new PhotoPanel(chat, url);
    chat->addPanel(photos);
    connect(chat->client(), SIGNAL(connected()), photos, SLOT(open()));

    // register shortcut
    QAction *action = chat->addAction(QIcon(":/photos.png"), photos->windowTitle());
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_P));
    connect(action, SIGNAL(triggered()),
            photos, SIGNAL(showPanel()));

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(photos, PhotosPlugin)
