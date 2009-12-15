/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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
#include <QPushButton>
#include <QProgressBar>
#include <QSystemTrayIcon>
#include <QUrl>

#include "photos.h"

#define PROGRESS_STEPS 100

PhotosList::PhotosList(QWidget *parent)
    : QListWidget(parent)
{
    setIconSize(QSize(24, 24));
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
    QListWidgetItem *dropItem = itemAt(event->pos());
    if (event->mimeData()->hasUrls() && dropItem)
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        emit filesDropped(event->mimeData()->urls(),
            dropItem->data(Qt::UserRole).toUrl());
    }
}

Photos::Photos(const QString &url, QWidget *parent)
    : QWidget(parent), busy(false), remoteUrl(url),
    systemTrayIcon(NULL)
{
    /* create UI */
    helpLabel = new QLabel(tr("To upload your photos, simply drag and drop them to a folder."));

    photosView = new QStackedWidget;
    PhotosList *listView = new PhotosList;
    photosView->addWidget(listView);
    connect(listView, SIGNAL(filesDropped(const QList<QUrl>&, const QUrl&)),
            this, SLOT(filesDropped(const QList<QUrl>&, const QUrl&)));
    connect(listView, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(chDir(QListWidgetItem *)));

    progressBar = new QProgressBar;
    progressBar->setRange(0, 0);
    progressBar->setValue(0);
    progressBar->hide();

    statusLabel = new QLabel(tr("Connecting.."));
    QPushButton *createButton = new QPushButton(tr("Create a folder"));
    createButton->setIcon(QIcon(":/add.png"));
    connect(createButton, SIGNAL(clicked()), this, SLOT(createFolder()));

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(helpLabel);
    layout->addWidget(photosView);
    layout->addWidget(progressBar);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(statusLabel);
    hbox->addWidget(createButton);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowIcon(QIcon(":/photos.png"));

    /* open filesystem */
    fs = FileSystem::factory(remoteUrl, this);
    connect(fs, SIGNAL(commandFinished(int, bool, const FileInfoList&)), this,
            SLOT(commandFinished(int, bool, const FileInfoList&)));
    connect(fs, SIGNAL(putProgress(int, int)), this, SLOT(putProgress(int, int)));
    fs->open(remoteUrl);
}

void Photos::chDir(QListWidgetItem *item)
{
    QUrl url = item->data(Qt::UserRole).value<QUrl>();
    qDebug() << "chDir" << url;
    remoteUrl = url.toString();
    if(remoteUrl.endsWith(".jpg")) {
        qDebug() << "Getting" << remoteUrl;
        fdPhoto = fs->get(remoteUrl);
    } else {
        PhotosList *listView = new PhotosList;
        photosView->setCurrentIndex(photosView->addWidget(listView));
        connect(listView, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
        this, SLOT(chDir(QListWidgetItem *)));
        fs->list(url.toString());
    }
}

void Photos::commandFinished(int cmd, bool error, const FileInfoList &results)
{
    if (error)
        qWarning() << fs->commandName(cmd) << "command failed";

    switch (cmd)
    {
    case FileSystem::Get: {
        if (error) {
            qDebug() << "Error Get" << remoteUrl;
            return;
        }
        fdPhoto->reset();
        QImage img;
        img.load(fdPhoto, NULL);
        img = img.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel *aLabel = new QLabel();
        //aLabel->setText("foobar");
        aLabel->setAlignment(Qt::AlignCenter);
        aLabel->setPixmap(QPixmap::fromImage(img));
        photosView->setCurrentIndex(photosView->addWidget(aLabel));
        fdPhoto->close();
        break;
    }
    case FileSystem::Open:
        if (!error)
            refresh();
        break;
    case FileSystem::List: {
        if (error) {
            qDebug() << "Error listing" << remoteUrl;
            return;
        }
        PhotosList *listView = qobject_cast<PhotosList *>(photosView->currentWidget());
        Q_ASSERT(listView != NULL);
        //listView->clear();
        foreach (const FileInfo& info, results)
        {
            QListWidgetItem *newItem = new QListWidgetItem;
            if (info.isDir()) {
                newItem->setIcon(QIcon(":/photos.png"));
            } else {
                qDebug() << info.name() << info.url();
            }
            newItem->setData(Qt::UserRole, QUrl(info.url()));
            newItem->setText(info.name());
            listView->addItem(newItem);
        }
        listView->show();

        /* drag and drop is now allowed */
        statusLabel->setText("");
        listView->setAcceptDrops(true);
        break;
    }
    case FileSystem::Mkdir:
        refresh();
        break;
    case FileSystem::Put:
        progressBar->setValue(PROGRESS_STEPS * ((progressBar->value() / PROGRESS_STEPS) + 1));
        busy = false;
        processQueue();
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
        fs->mkdir(remoteUrl + "/" + text);
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
        queue.append(QPair<QUrl, QUrl>(url, base + "/" + file.fileName()));
    }
    progressBar->setMaximum(progressBar->maximum() + PROGRESS_STEPS * files.size());
    progressBar->show();
    processQueue();
}

void Photos::processQueue()
{
    if (busy)
        return;

    /* if the queue is empty, hide progress bar and reset it */
    if (queue.empty())
    {
        statusLabel->setText(tr("Photos upload complete."));
        if (systemTrayIcon)
            systemTrayIcon->showMessage(tr("Photos upload complete."),
                tr("Your photos have been uploaded."));
        progressBar->hide();
        progressBar->setValue(0);
        progressBar->setMaximum(0);
        return;
    }

    /* process the next file to upload */
    QPair<QUrl, QUrl> item = queue.takeFirst();
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
    fs->list(remoteUrl);
}

void Photos::setSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    systemTrayIcon = trayIcon;
}

