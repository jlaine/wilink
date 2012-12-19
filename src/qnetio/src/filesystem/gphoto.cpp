/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QThread>

#include "gphoto2/gphoto2.h"

#include "gphoto.h"

class ListThread : public QThread
{
public:
    ListThread(QObject *parent, Camera *getCamera, GPContext *getContext, const QUrl &getUrl);
    void run();

    bool error;
    FileInfoList results;

protected:
    Camera *camera;
    GPContext *context;
    QBuffer *output;
    QUrl url;
};

ListThread::ListThread(QObject *parent, Camera *getCamera, GPContext *getContext, const QUrl &getUrl)
    : error(false), camera(getCamera), context(getContext), url(getUrl)
{
}

void ListThread::run()
{
    CameraList *list;
    const char *name;

    gp_list_new(&list);

    /* list folders */
    if (gp_camera_folder_list_folders(camera, QFile::encodeName(url.path()), list, context) == GP_OK) {
        int count = gp_list_count(list);
        for (int i = 0; i < count; i++)
        {
            if (gp_list_get_name(list, i, &name) == GP_OK)
            {
                QString itemName = QString::fromLocal8Bit(name);
                FileInfo item;
                item.setDir(true);
                item.setName(itemName);
                item.setUrl(FileInfo::fileUrl(url, itemName));
                results.append(item);
            }
        }
    } else {
        qWarning("Could not list camera folders");
        error = true;
    }
    gp_list_reset(list);

    /* list files */
    if (gp_camera_folder_list_files(camera, QFile::encodeName(url.path()), list, context) == GP_OK) {
        int count = gp_list_count(list);
        for (int i = 0; i < count; i++) {
            if (gp_list_get_name(list, i, &name) == GP_OK)
            {
                QString itemName = QString::fromLocal8Bit(name);
                FileInfo item;
                item.setDir(false);
                item.setName(itemName);
                item.setUrl(FileInfo::fileUrl(url, itemName));

                CameraFileInfo info;
                if (gp_camera_file_get_info(camera, QFile::encodeName(url.path()), name, &info, context) == GP_OK)
                {
                    item.setSize(info.file.size);
                }
                results.append(item);
            }
        }
    } else {
        qWarning("Could not list camera files");
        error = true;
    }

    gp_list_free(list);
}

GPhotoFileSystem::GPhotoFileSystem(QObject *parent)
    : FileSystem(parent), camera(0), context(0)
{
    context = gp_context_new();
    gp_camera_new(&camera);
}

GPhotoFileSystem::~GPhotoFileSystem()
{
    gp_camera_unref(camera);
    gp_context_unref(context);
}

FileSystemJob* GPhotoFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::List, this);

    QStringList filePath = fileUrl.path().split("/");
    QString fileName = filePath.last();
    filePath.removeLast();

    bool ok = false;
    CameraFile *file;
    gp_file_new (&file);
    if (gp_camera_file_get(camera, QFile::encodeName(filePath.join("/")), QFile::encodeName(fileName), GP_FILE_TYPE_NORMAL, file, context) == GP_OK)
    {
        const char *data = 0;
        unsigned long size = 0;
        if (gp_file_get_data_and_size(file, &data, &size) == GP_OK)
        {
            QBuffer *buffer = new QBuffer(job);
            buffer->setData(data, size);
            job->setData(buffer);
            ok = true;
        }
    }
    gp_file_unref(file);

    job->finishLater(ok ? FileSystemJob::NoError : FileSystemJob::UnknownError);
    return job;
}

FileSystemJob* GPhotoFileSystem::open(const QUrl &url)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Open, this);

#if 0
    GPPortInfoList *infoList;
    GPPortInfo info;

    gp_port_info_list_new(&infoList);
    gp_port_info_list_load(infoList);

    int nPorts = gp_port_info_list_count(infoList);
    qDebug() << "found" << nPorts << "ports";
    for (int i = 0; i < nPorts; i++)
    {
        gp_port_info_list_get_info(infoList, i, &info);
        qDebug() << "port" << i << info.name << info.path;
    }
    gp_port_info_list_free(infoList);
#endif

    if (gp_camera_init(camera, context) != GP_OK)
    {
        qWarning("Could not initialise camera");
        job->finishLater(FileSystemJob::UnknownError);
    } else {
        job->finishLater(FileSystemJob::NoError);
    }

    return job;
}

FileSystemJob* GPhotoFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::List, this);
    QThread *thread = new ListThread(this, camera, context, dirUrl);
    connect(thread, SIGNAL(finished()), this, SLOT(listFinished()));
    thread->start();
    return job;
}

class GPhotoFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("gphoto"))
            return new GPhotoFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(gphoto_filesystem, GPhotoFileSystemPlugin)
