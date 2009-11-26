/*
 * wDesktop
 * Copyright (C) 2008-2009 Bolloré telecom
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

#ifndef __BOC_PHOTOS_H__
#define __BOC_PHOTOS_H__

#include <QWidget>
#include <QListWidget>
#include <QUrl>

#include "qnetio/filesystem.h"

using namespace QNetIO;

class QLabel;
class QProgressBar;

class PhotosList : public QListWidget
{
    Q_OBJECT

public:
    PhotosList(QWidget *parent = NULL);

signals:
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
};

/** A TrayIcon is a system tray icon for interacting with a Panel.
 */
class Photos : public QWidget
{
    Q_OBJECT

public:
    Photos(const QString &url, const QIcon &folderIcon, QWidget *parent = NULL);

protected:
    void processQueue();
    void refresh();

protected slots:
    void createFolder();
    void commandFinished(int cmd, bool error, const FileInfoList &results);
    void filesDropped(const QList<QUrl> &files, const QUrl &destination);
    void putProgress(int done, int total);

private:
    bool busy;
    FileSystem *fs;
    QList< QPair<QUrl, QUrl> > queue;

    QIcon icon;
    QLabel *helpLabel;
    PhotosList *listView;
    QProgressBar *progressBar;
    QString remoteUrl;
    QLabel *statusLabel;
};

#endif
