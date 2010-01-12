/*
 * wDesktop
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

#include <QDebug>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QUrl>

#include <QScrollArea>
#include "photos.h"

static const int PROGRESS_STEPS = 100;
static const QSize ICON_SIZE(128, 128);

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

void PhotosList::setEntries(const FileInfoList &entries)
{
    fileList = entries;
    clear();
    foreach (const FileInfo& info, fileList)
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        if (info.isDir()) {
            newItem->setIcon(QIcon(":/photos.png"));
        } else {
            QPixmap blank(ICON_SIZE);
            blank.fill();
            newItem->setIcon(blank);
        }
        newItem->setData(Qt::UserRole, QUrl(info.url()));
        newItem->setText(info.name());
        addItem(newItem);
    }
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
}

QUrl PhotosList::url()
{
    return baseUrl;
}

Photos::Photos(const QString &url, QWidget *parent)
    : QWidget(parent), busy(false),
    systemTrayIcon(NULL)
{
    /* create UI */
    helpLabel = new QLabel(tr("To upload your photos, simply drag and drop them to a folder."));

    photosView = new QStackedWidget;
    PhotosList *listView = new PhotosList(url);
    listView->setBaseDrop(false);
    photosView->addWidget(listView);
    connect(listView, SIGNAL(filesDropped(const QList<QUrl>&, const QUrl&)),
            this, SLOT(filesDropped(const QList<QUrl>&, const QUrl&)));
    connect(listView, SIGNAL(folderOpened(const QUrl&)),
            this, SLOT(folderOpened(const QUrl&)));

    progressBar = new QProgressBar;
    progressBar->setRange(0, 0);
    progressBar->setValue(0);
    progressBar->hide();

    statusLabel = new QLabel(tr("Connecting.."));

    backButton = new QPushButton(tr("Go back"));
    backButton->setIcon(QIcon(":/back.png"));
    backButton->setEnabled(false);
    connect(backButton, SIGNAL(clicked()), this, SLOT(goBack()));

    QPushButton *createButton = new QPushButton(tr("Create a folder"));
    createButton->setIcon(QIcon(":/add.png"));
    connect(createButton, SIGNAL(clicked()), this, SLOT(createFolder()));

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(helpLabel);
    layout->addWidget(photosView);
    layout->addWidget(progressBar);
    QHBoxLayout *hbox = new QHBoxLayout;
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
    fs->open(url);
}

void Photos::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    if (error)
        qWarning() << fs->commandName(cmd) << "command failed";

    switch (cmd)
    {
    case FileSystem::Get:
        if (!error)
        {
            /* load image */
            fdPhoto->reset();
            QImage img;
            img.load(fdPhoto, NULL);
            fdPhoto->close();

            /* display image */
            PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
            Q_ASSERT(listView != NULL);
            listView->setImage(downloadUrl, img);
        }
        /* fetch next thumbnail */
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
        statusLabel->setText("");
        listView->setAcceptDrops(true);
        if (photosView->count() > 1)
            backButton->setEnabled(true);

        /* fetch thumbnails */
        foreach (const FileInfo& info, results)
        {
            if (!info.isDir() && info.name().endsWith(".jpg"))
                downloadQueue.append(info.url());
        }
        processDownloadQueue();
        break;
    }
    case FileSystem::Mkdir:
        refresh();
        break;
    case FileSystem::Put:
        progressBar->setValue(PROGRESS_STEPS * ((progressBar->value() / PROGRESS_STEPS) + 1));
        busy = false;
        processUploadQueue();
        break;
    default:
        qWarning() << fs->commandName(cmd) << "was not expected";
        break;
    }
}

void Photos::createFolder()
{
     bool ok;
     QString text = QInputDialog::getText(this, tr("Create a folder"),
                                          tr("Folder name:"), QLineEdit::Normal,
                                          "", &ok);
    if (ok && !text.isEmpty())
    {
        PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
        Q_ASSERT(listView != NULL);
        listView->setAcceptDrops(false);
        fs->mkdir(listView->url().toString() + "/" + text);
    }
}

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

void Photos::folderOpened(const QUrl &url)
{
    qDebug() << "folderOpened" << url;

    PhotosList *listView = new PhotosList(url);
    photosView->setCurrentIndex(photosView->addWidget(listView));
    connect(listView, SIGNAL(filesDropped(const QList<QUrl>&, const QUrl&)),
            this, SLOT(filesDropped(const QList<QUrl>&, const QUrl&)));
    connect(listView, SIGNAL(folderOpened(const QUrl&)),
        this, SLOT(folderOpened(const QUrl&)));
    fs->list(url.toString());
}

void Photos::goBack()
{
    if (photosView->count() <= 1)
        return;
    photosView->removeWidget(photosView->currentWidget());
    if (photosView->count() <= 1)
        backButton->setEnabled(false);
}

void Photos::processDownloadQueue()
{
    if (downloadQueue.empty())
        return;

    downloadUrl = downloadQueue.takeFirst();
    fdPhoto = fs->get(downloadUrl);
}

void Photos::processUploadQueue()
{
    if (busy)
        return;

    /* if the queue is empty, hide progress bar and reset it */
    if (uploadQueue.empty())
    {
        statusLabel->setText(tr("Photos upload complete."));
        if (systemTrayIcon)
            systemTrayIcon->showMessage(tr("Photos upload complete."),
                tr("Your photos have been uploaded."));
        progressBar->hide();
        progressBar->setValue(0);
        progressBar->setMaximum(0);

        /* refresh the current view */
        refresh();
        return;
    }

    /* process the next file to upload */
    QPair<QUrl, QUrl> item = uploadQueue.takeFirst();
    QFile *file = new QFile(item.first.toLocalFile(), this);
    statusLabel->setText(tr("Uploading %1").arg(QFileInfo(file->fileName()).fileName()));
    busy = true;
    fs->put(file, item.second.toString());
}

void Photos::putProgress(int done, int total)
{
    if (!total)
        return;
    progressBar->setValue(PROGRESS_STEPS * (int(progressBar->value() / PROGRESS_STEPS) + double(done) / double(total)));
}

void Photos::refresh()
{
    statusLabel->setText(tr("Loading your folders.."));
    PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
    Q_ASSERT(listView != NULL);
    fs->list(listView->url());
}

void Photos::setSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    systemTrayIcon = trayIcon;
}

