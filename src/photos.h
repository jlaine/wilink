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

#ifndef __PHOTOS_H__
#define __PHOTOS_H__

#include <QWidget>
#include <QListWidget>
#include <QUrl>

#include "qnetio/filesystem.h"

using namespace QNetIO;

class QImage;
class QLabel;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QSystemTrayIcon;

class PhotosList : public QListWidget
{
    Q_OBJECT

public:
    PhotosList(const QUrl &url, QWidget *parent = NULL);
    void setBaseDrop(bool accept);
    void setEntries(const FileInfoList &entries);
    void setImage(const QUrl &url, const QImage &img);
    QUrl url();

signals:
    void fileOpened(const QUrl &url);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    FileInfo itemEntry(QListWidgetItem *item);

protected slots:
    void slotItemDoubleClicked(QListWidgetItem *item);
    void slotReturnPressed();

private:
    bool baseDrop;
    QUrl baseUrl;
    FileInfoList fileList;
};

/** Main window for photos application
 */
class Photos : public QWidget
{
    Q_OBJECT

    class Job {
    public:
        Job() : type(-1), widget(0) {};
        Job(QWidget *downloadWidget, const QUrl &downloadUrl, int downloadType)
            : remoteUrl(downloadUrl), type(downloadType), widget(downloadWidget) {};
        void clear() { remoteUrl.clear(); type = -1; widget = 0; };
        bool isEmpty() { return remoteUrl.isEmpty() || type < 0; };

        QUrl remoteUrl;
        int type;
        QWidget *widget;
    };

public:
    Photos(const QString &url, QWidget *parent = NULL);
    void setSystemTrayIcon(QSystemTrayIcon *trayIcon);

protected:
    void processDownloadQueue();
    void processUploadQueue();
    void showMessage(const QString &message = QString());

protected slots:
    void createFolder();
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void fileOpened(const QUrl &url);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);
    void goBack();
    void pushView(QWidget *widget);
    void putProgress(int done, int total);
    void refresh();

private:
    bool busy;
    FileSystem *fs;
    QList<Job> downloadQueue;
    QList< QPair<QUrl, QUrl> > uploadQueue;

    QPushButton *backButton;
    QPushButton *createButton;
    Job downloadJob;
    QLabel *helpLabel;
    QStackedWidget *photosView;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QIODevice *fdPhoto;
};

#endif
