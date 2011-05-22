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

#ifndef __WILINK_PHOTOS_H__
#define __WILINK_PHOTOS_H__

#include <QListWidget>
#include <QUrl>

#include "chat_model.h"
#include "chat_panel.h"
#include "qnetio/filesystem.h"

using namespace QNetIO;

class Chat;
class PhotoUploadItem;
class PhotoUploadModel;
class QImage;
class QLabel;
class QProgressBar;
class QPushButton;
class QStackedWidget;

class PhotosList : public QListWidget
{
    Q_OBJECT

public:
    PhotosList(const QUrl &url, QWidget *parent = NULL);
    void setBaseDrop(bool accept);
    FileInfoList entries() const;
    void setEntries(const FileInfoList &entries);
    void setImage(const QUrl &url, const QImage &img);
    FileInfoList selectedEntries() const;
    QUrl url();

signals:
    void fileOpened(const QUrl &url);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    FileInfo itemEntry(QListWidgetItem *item) const;

protected slots:
    void slotItemDoubleClicked(QListWidgetItem *item);
    void slotReturnPressed();

private:
    bool baseDrop;
    QUrl baseUrl;
    FileInfoList fileList;
};

class PhotoCache : public QObject
{
    Q_OBJECT

public:
    static PhotoCache *instance();
    QUrl imageUrl(const QUrl &url, FileSystem *fs);

signals:
    void photoChanged(const QUrl &url);

private slots:
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void processQueue();

private:
    PhotoCache();
    QSet<FileSystem*> m_fileSystems;
    QList<QPair<QUrl, FileSystem*> > m_downloadQueue;
    QUrl m_downloadUrl;
    QIODevice *m_downloadDevice;
};

class PhotoModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl rootUrl READ rootUrl WRITE setRootUrl NOTIFY rootUrlChanged)
    Q_PROPERTY(PhotoUploadModel* uploads READ uploads CONSTANT)

public:
    PhotoModel(QObject *parent = 0);

    QUrl rootUrl() const;
    void setRootUrl(const QUrl &rootUrl);

    PhotoUploadModel *uploads() const;

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void rootUrlChanged(const QUrl &rootUrl);

public slots:
    void refresh();
    void upload(const QString &filePath);

private slots:
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void photoChanged(const QUrl &url);

private:
    FileSystem *m_fs;
    QUrl m_rootUrl;
    PhotoUploadModel *m_uploads;
};

class PhotoUploadModel : public ChatModel
{
    Q_OBJECT

public:
    PhotoUploadModel(QObject *parent = 0);

    void append(const QString &sourcePath, FileSystem *fileSystem, const QString &destinationPath);

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private slots:
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void processQueue();
    void putProgress(int done, int total);

private:
    PhotoModel *m_photoModel;
    PhotoUploadItem *m_uploadItem;
    QIODevice *m_uploadDevice;
};

/** The PhotoPanel class represents a panel for displaying photos.
 */
class PhotoPanel : public ChatPanel
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
    PhotoPanel(Chat *chatWindow, const QString &url);

protected:
    void processDownloadQueue();
    void processUploadQueue();
    void showMessage(const QString &message = QString());

protected slots:
    void abortUpload();
    void createFolder();
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void deleteFile();
    void fileNext();
    void fileOpened(const QUrl &url);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void folderOpened(const QUrl &url);
    void goBack();
    void open();
    void pushView(QWidget *widget);
    void putProgress(int done, int total);
    void refresh();

private:
    FileSystem *fs;
    QUrl baseUrl;
    QList<Job> downloadQueue;
    QIODevice *downloadDevice;
    QList< QPair<QUrl, QUrl> > uploadQueue;
    QIODevice *uploadDevice;
    QList<QUrl> playList;

    QAction *backAction;
    QAction *createAction;
    QAction *deleteAction;
    QPushButton *stopButton;
    Job downloadJob;
    QStackedWidget *photosView;
    int progressFiles;
    QProgressBar *progressBar;
    QLabel *statusLabel;
};

#endif
