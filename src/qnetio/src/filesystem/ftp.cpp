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
#include <QDir>
#include <QFile>
#include <QUrl>

#include "ftp.h"
#include "wallet.h"

FtpFileSystem::FtpFileSystem(QObject *parent)
    : FileSystem(parent),
    currentJob(0),
    isOpen(false)
{
    connect(&ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(onCommandFinished(int,bool)));
    connect(&ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(onListInfo(QUrlInfo)));
}

FileSystemJob *FtpFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(type);
    currentJob = new FileSystemJob(FileSystemJob::Get, this);
    QIODevice *output = new QBuffer(currentJob);
    if (!output->open(QIODevice::ReadWrite)) {
        currentJob->finishLater(FileSystemJob::UnknownError);
        return currentJob;
    }
    currentJob->setData(output);
    connect(&ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            currentJob, SLOT(downloadProgress(qint64,qint64)));
    ftp.get(fileUrl.path(), output);
    return currentJob;
}

FileSystemJob* FtpFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(filter);
    currentJob = new FileSystemJob(FileSystemJob::List, this);
    currentUrl = dirUrl;
    listItems.clear();
    ftp.list(currentUrl.path());
    return currentJob;
}

FileSystemJob* FtpFileSystem::mkdir(const QUrl &dirUrl)
{
    currentJob = new FileSystemJob(FileSystemJob::Mkdir, this);
    ftp.mkdir(dirUrl.path());
    return currentJob;
}

FileSystemJob* FtpFileSystem::open(const QUrl &url)
{
    currentJob = new FileSystemJob(FileSystemJob::Open, this);
    currentHost = url.host();
    ftp.connectToHost(currentHost);
    return currentJob;
}

FileSystemJob* FtpFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    currentJob = new FileSystemJob(FileSystemJob::Put, this);
    connect(&ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            currentJob, SLOT(uploadProgress(qint64,qint64)));
    ftp.put(data, fileUrl.path());
    return currentJob;
}

FileSystemJob* FtpFileSystem::remove(const QUrl &fileUrl)
{
    currentJob = new FileSystemJob(FileSystemJob::Remove, this);
    ftp.remove(fileUrl.path());
    return currentJob;
}

FileSystemJob* FtpFileSystem::rmdir(const QUrl &dirUrl)
{
    currentJob = new FileSystemJob(FileSystemJob::Rmdir, this);
    ftp.rmdir(dirUrl.path());
    return currentJob;
}

void FtpFileSystem::onCommandFinished(int, bool error)
{
    switch (ftp.currentCommand())
    {
    case QFtp::ConnectToHost:
        if (error) {
            currentJob->setError(FileSystemJob::UnknownError);
            QMetaObject::invokeMethod(currentJob, "finished");
            currentJob = 0;
        } else {
            QAuthenticator auth;
            auth.setUser("anonymous");
            Wallet::instance()->onAuthenticationRequired(currentHost, &auth);
            ftp.login(auth.user(), auth.password());
        }
        break;
    case QFtp::List:
        if (error)
            currentJob->setError(FileSystemJob::UnknownError);
        else
            currentJob->setResults(listItems);
        QMetaObject::invokeMethod(currentJob, "finished");
        currentJob = 0;
        break;
    case QFtp::Get:
    case QFtp::Login:
    case QFtp::Mkdir:
    case QFtp::Put:
    case QFtp::Remove:
    case QFtp::Rmdir:
        if (error)
            currentJob->setError(FileSystemJob::UnknownError);
        QMetaObject::invokeMethod(currentJob, "finished");
        currentJob = 0;
        break;
    default:
        break;
    }
}

void FtpFileSystem::onListInfo(const QUrlInfo &i)
{
    QString base = currentUrl.toString();
    if (!base.endsWith("/"))
        base += "/";

    FileInfo item;
    item.setUrl(base + i.name());
    item.setName(i.name());
    item.setDir(i.isDir());
    item.setLastModified(i.lastModified());
    item.setSize(i.size());
    listItems.append(item);
}

class FtpFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("ftp"))
            return new FtpFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(ftp_filesystem, FtpFileSystemPlugin)
