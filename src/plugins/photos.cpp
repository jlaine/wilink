/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#include <QApplication>
#include <QBuffer>
#include <QDebug>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileIconProvider>
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
#include <QTimer>
#include <QUrl>

#include "chat.h"
#include "chat_plugin.h"
#include "photos.h"

static const int PROGRESS_STEPS = 100;
static const QSize ICON_SIZE(128, 128);
static const QSize UPLOAD_SIZE(2048, 2048);

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

    /* register panel */
    QTimer::singleShot(0, this, SIGNAL(registerPanel()));
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

FileInfo PhotosList::itemEntry(QListWidgetItem *item)
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
    QFileIconProvider iconProvider;
    fileList = entries;
    clear();
    foreach (const FileInfo& info, fileList)
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        if (info.isDir()) {
            newItem->setIcon(iconProvider.icon(QFileIconProvider::Folder));
        } else {
            newItem->setIcon(iconProvider.icon(QFileIconProvider::File));
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

Photos::Photos(const QString &url, QWidget *parent)
    : ChatPanel(parent),
    downloadDevice(0),
    uploadDevice(0),
    progressFiles(0),
    systemTrayIcon(NULL)
{
    /* create UI */
    helpLabel = new QLabel(tr("To upload your photos to wifirst.net, simply drag and drop them to an album."));
    helpLabel->setWordWrap(true);

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

    backButton = new QPushButton(tr("Go back"));
    backButton->setIcon(QIcon(":/back.png"));
    backButton->setEnabled(false);
    connect(backButton, SIGNAL(clicked()), this, SLOT(goBack()));

    createButton = new QPushButton(tr("Create an album"));
    createButton->setIcon(QIcon(":/add.png"));
    connect(createButton, SIGNAL(clicked()), this, SLOT(createFolder()));

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addItem(headerLayout());
    layout->addWidget(helpLabel);
    layout->addWidget(photosView);
    layout->addWidget(progressBar);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(backButton);
    hbox->addWidget(createButton);
    layout->addItem(hbox);
    layout->addWidget(statusLabel);

    setLayout(layout);
    setWindowIcon(QIcon(":/photos.png"));
    setWindowTitle(tr("Upload photos"));

    /* open filesystem */
    fs = FileSystem::factory(url, this);
    connect(fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)), this,
            SLOT(commandFinished(int, bool, const FileInfoList&)));
    connect(fs, SIGNAL(putProgress(int, int)), this, SLOT(putProgress(int, int)));
    showMessage(tr("Connecting.."));
    fs->open(url);

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(goBack()));

    setFocusProxy(photosView);
    resize(QSize(600, 400).expandedTo(minimumSizeHint()));
}

/** When a command finishes, process its results.
 *
 * @param cmd
 * @param error
 * @param results
 */
void Photos::commandFinished(int cmd, bool error, const FileInfoList &results)
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
                QSize maxSize(800, 600);
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
            backButton->setEnabled(true);

        /* fetch thumbnails */
        foreach (const FileInfo& info, results)
        {
            if (!info.isDir() && info.name().endsWith(".jpg"))
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
    default:
        qWarning() << fs->commandName(cmd) << "was not expected";
        break;
    }
}

/** Prompt the user for a new folder name and create it.
 */
void Photos::createFolder()
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

void Photos::fileNext()
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
void Photos::fileOpened(const QUrl &url)
{
    if (!url.toString().endsWith(".jpg"))
        return;

    // disable controls
    createButton->setEnabled(false);
    helpLabel->hide();

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
void Photos::filesDropped(const QList<QUrl> &files, const QUrl &destination)
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
    processUploadQueue();
}

/** Open the remote folder at the given URL.
 *
 * @param url
 */
void Photos::folderOpened(const QUrl &url)
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
void Photos::goBack()
{
    if (photosView->count() < 2 || !backButton->isEnabled())
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
        backButton->setEnabled(false);
    createButton->setEnabled(true);
    helpLabel->show();
}

/** If the download queue is not empty, process the next item.
 */
void Photos::processDownloadQueue()
{
    if (downloadQueue.empty() || !downloadJob.isEmpty())
        return;

    downloadJob = downloadQueue.takeFirst();
    downloadDevice = fs->get(downloadJob.remoteUrl, downloadJob.type);
}

/** If the upload queue is not empty, process the next item.
 */
void Photos::processUploadQueue()
{
    if (uploadDevice)
        return;

    /* if the queue is empty, hide progress bar and reset it */
    if (uploadQueue.empty())
    {
        showMessage(tr("Photos upload complete."));
        if (systemTrayIcon)
            systemTrayIcon->showMessage(tr("Photos upload complete."),
                tr("Your photos have been uploaded."));
        progressBar->hide();
        progressBar->setValue(0);
        progressBar->setMaximum(0);
        progressFiles = 0;

        /* refresh the current view */
        refresh();
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

void Photos::pushView(QWidget *widget)
{
    static int counter = 0;
    widget->setObjectName(QString::number(counter++));
    photosView->setCurrentIndex(photosView->addWidget(widget));
}

/** Update the progress bar for the current upload.
 */
void Photos::putProgress(int done, int total)
{
    if (!total)
        return;
    progressBar->setValue(PROGRESS_STEPS * (progressFiles + double(done) / double(total)));
}

/** Refresh the contents of the current folder.
 */
void Photos::refresh()
{
    PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
    if (!listView)
        return;

    showMessage(tr("Loading your albums.."));
    listView->setAcceptDrops(false);
    fs->list(listView->url());
}

/** Set the system tray icon.
 *
 * @param trayIcon
 */
void Photos::setSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    systemTrayIcon = trayIcon;
}

void Photos::showMessage(const QString &message)
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
};

bool PhotosPlugin::initialize(Chat *chat)
{
    QString url;
    QString domain = chat->chatClient()->getConfiguration().domain();
    if (domain == "wifirst.net")
        url = "wifirst://www.wifirst.net/w";
    else if (domain == "gmail.com")
        url = "picasa://default";
    else
        return false;

    /* register panel */
    Photos *photos = new Photos(url);
    //photos->setSystemTrayIcon(this);
    photos->setObjectName("photos");
    chat->addPanel(photos);

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_P), chat);
    connect(shortcut, SIGNAL(activated()), photos, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(photos, PhotosPlugin)
