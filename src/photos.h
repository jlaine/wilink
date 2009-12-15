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

#ifndef __PHOTOS_H__
#define __PHOTOS_H__

#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QUrl>

#include "qnetio/filesystem.h"

using namespace QNetIO;

class QImage;
class QLabel;
class QProgressBar;
class QSystemTrayIcon;

class PhotosList : public QListWidget
{
    Q_OBJECT

public:
    PhotosList(const QUrl &url, QWidget *parent = NULL);
    void setEntries(const FileInfoList &entries);
    void setImage(const QUrl &url, const QImage &img);
    QUrl url();

signals:
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    FileInfo itemEntry(QListWidgetItem *item);

protected slots:
    void slotItemDoubleClicked(QListWidgetItem *item);

private:
    QUrl baseUrl;
    FileInfoList fileList;
};

/** Main window for photos application
 */
class Photos : public QWidget
{
    Q_OBJECT

public:
    Photos(const QString &url, QWidget *parent = NULL);
    void setSystemTrayIcon(QSystemTrayIcon *trayIcon);

protected:
    void processDownloadQueue();
    void processUploadQueue();
    void refresh();

protected slots:
    void createFolder();
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);
    void putProgress(int done, int total);

private:
    bool busy;
    FileSystem *fs;
    QList<QUrl> downloadQueue;
    QList< QPair<QUrl, QUrl> > uploadQueue;

    QUrl downloadUrl;
    QLabel *helpLabel;
    QStackedWidget *photosView;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QIODevice *fdPhoto;
};

#endif
