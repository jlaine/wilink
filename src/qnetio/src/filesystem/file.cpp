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

#include <QUrl>

#include "file.h"

LocalFileSystem::LocalFileSystem(QObject *parent)
    : FileSystem(parent)
{
}

FileSystemJob* LocalFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(type);
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Get, this);
    QFile *file = new QFile(fileUrl.toLocalFile());
    file->setParent(job);
    if (file->open(QIODevice::ReadOnly)) {
        job->setData(file);
        job->finishLater(FileSystemJob::NoError);
    } else {
        job->finishLater(FileSystemJob::UnknownError);
    }
    return job;
}

FileSystemJob* LocalFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(filter);
    FileSystemJob *job = new FileSystemJob(FileSystemJob::List, this);

    QFileInfoList entries;
    QFileInfo info(dirUrl.toLocalFile());
    if (info.isDir())
        entries = QDir(dirUrl.toLocalFile()).entryInfoList(QStringList("*"));
    else
        entries.append(info);

    FileInfoList results;
    foreach (const QFileInfo &entry, entries) {
        if (entry.fileName() != "." && entry.fileName() != "..") {
            FileInfo item;
            item.setUrl(QUrl::fromLocalFile(entry.absoluteFilePath()).toString());
            item.setName(entry.fileName());
            item.setDir(entry.isDir());
            item.setLastModified(entry.lastModified());
            item.setSize(entry.size());
            results.append(item);
        }
    }

    job->setResults(results);
    job->finishLater(FileSystemJob::NoError);
    return job;
}

FileSystemJob* LocalFileSystem::mkdir(const QUrl &dirUrl)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Mkdir, this);

    QFileInfo info(dirUrl.toLocalFile());
    if (info.absoluteDir().mkdir(info.fileName()))
        job->finishLater(FileSystemJob::NoError);
    else
        job->finishLater(FileSystemJob::UnknownError);
    return job;
}

FileSystemJob* LocalFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Put, this);

    QFile file(fileUrl.toLocalFile());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data->readAll());
        job->finishLater(FileSystemJob::NoError);
    } else {
        job->finishLater(FileSystemJob::UnknownError);
    }
    return job;
}

FileSystemJob* LocalFileSystem::open(const QUrl &url)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Open, this);
    if (QFileInfo(url.toLocalFile()).exists())
        job->finishLater(FileSystemJob::NoError);
    else
        job->finishLater(FileSystemJob::UnknownError);
    return job;
}

FileSystemJob* LocalFileSystem::remove(const QUrl &fileUrl)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Remove, this);
    if (QFile(fileUrl.toLocalFile()).remove())
        job->finishLater(FileSystemJob::NoError);
    else
        job->finishLater(FileSystemJob::UnknownError);
    return job;
}

FileSystemJob* LocalFileSystem::rmdir(const QUrl &dirUrl)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Rmdir, this);
    QFileInfo info(dirUrl.toLocalFile());
    if (info.absoluteDir().rmdir(info.fileName()))
        job->finishLater(FileSystemJob::NoError);
    else
        job->finishLater(FileSystemJob::UnknownError);
    return job;
}

QNetIO::FileSystem *LocalFileSystemPlugin::create(const QUrl &url, QObject *parent)
{
    if (url.scheme() == QLatin1String("file"))
        return new LocalFileSystem(parent);
    return NULL;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_STATIC_PLUGIN2(local_filesystem, LocalFileSystemPlugin)
#endif
