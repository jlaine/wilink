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

#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

#include "wifirst.h"
#include "mime.h"
#include "wallet.h"

static QDateTime parseDateTime(const QString &timestr)
{
    // example: 2007-09-22T13:16:16+02:00
    QRegExp format("(.+)([-+])([0-9]+):([0-9]+)");
    if (format.exactMatch(timestr))
    {
        QDateTime dt = QDateTime::fromString(format.cap(1), Qt::ISODate);
        if (dt.isValid())
        {
            int off = (format.cap(2) == "+" ? -1 : 1) * (format.cap(3).toInt() * 3600 + format.cap(4).toInt() * 60);
            dt = dt.addSecs(off);
            dt.setTimeSpec(Qt::UTC);
            return dt;
        }
    }
    return QDateTime();
}

class WifirstFileSystemJob : public FileSystemJob
{
public:
    WifirstFileSystemJob(Operation op, WifirstFileSystem *fs)
        : FileSystemJob(op, fs), m_fs(fs)
    {
    }

public:
    QUrl baseUrl;

protected:
    void networkFinished(QNetworkReply *reply);

private:
    WifirstFileSystem* m_fs;
};

void WifirstFileSystemJob::networkFinished(QNetworkReply *reply)
{
    switch (operation())
    {
    case List:
        if (reply->error() == QNetworkReply::NoError) {
            QDomDocument doc;
            doc.setContent(reply);
            if (doc.documentElement().tagName() == "albums")
            {
                // root folder
                QDomElement album = doc.documentElement().firstChildElement("album");
                FileInfoList results;
                while (!album.isNull()) {
                    results.append(m_fs->parseAlbum(album));
                    album = album.nextSiblingElement("album");
                }
                setAllowedOperations(List | Mkdir | Rmdir);
                setResults(results);
            } else {
                // album folder
                setAllowedOperations(List | Put);
                setResults(m_fs->parseFiles(doc.documentElement(), property("__base_url").toString()));
            }
        }
        break;
    case Mkdir:
        if (reply->error() == QNetworkReply::NoError) {
            QDomDocument doc;
            doc.setContent(reply);
            m_fs->parseAlbum(doc.documentElement());
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    case Put:
        if (reply->error() == QNetworkReply::NoError) {
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    case Remove:
        if (reply->error() == QNetworkReply::NoError) {
            m_fs->pictureHash.remove(property("__picture_id").toInt());
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    case Rmdir:
        if (reply->error() == QNetworkReply::NoError) {
            m_fs->albumHash.remove(property("__album_name").toString());
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    default:
        break;
    }

    if (reply->error() != QNetworkReply::NoError) {
        setError(UnknownError);
        setErrorString(reply->errorString());
    }
    emit finished();
}

WifirstFileSystem::WifirstFileSystem(QObject *parent)
    : FileSystem(parent)
{
    network = createNetworkAccessManager(this);
}

FileSystemJob* WifirstFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    FileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Get, this);

    // parse url
    QRegExp regex("^/([^/]+)/([0-9]+).jpg$");
    const QString urlString = fileUrl.toString();
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())) || pictureHash[regex.cap(2).toInt()].isEmpty())
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    QString url = pictureHash[regex.cap(2).toInt()];
    if (type == SmallSize)
        url = url.replace("_original", "_small");
    else if (type == MediumSize)
        url = url.replace("_original", "_medium");
    else if (type == LargeSize)
        url = url.replace("_original", "_big");

    // NOTE: this is the only request for which we don't send
    // an Accept header.
    QNetworkReply *reply = network->get(QNetworkRequest(url));
    job->setData(reply);
    job->setNetworkReply(reply);
    return job;
}

FileSystemJob* WifirstFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(filter);
    FileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::List, this);
    const QString urlString = dirUrl.toString();
    if (urlString == baseVirtual)
    {
        QNetworkRequest req(FileInfo::fileUrl(baseUrl, "albums"));
        req.setRawHeader("Accept", "application/xml");
        job->setNetworkReply(network->get(req));
        return job;
    }

    /* parse url */
    int albumId = 0;
    QRegExp regex("^/([^/]+)$");
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())) || !(albumId = albumHash[regex.cap(1)]))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    /* send request */
    QNetworkRequest req(FileInfo::fileUrl(baseUrl, QString("albums/%1/pictures").arg(albumId)));
    req.setRawHeader("Accept", "application/xml");
    job->setProperty("__base_url", urlString);
    job->setNetworkReply(network->get(req));
    return job;
}

FileSystemJob* WifirstFileSystem::mkdir(const QUrl &dirUrl)
{
    WifirstFileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Mkdir, this);

    // parse URL
    const QString urlString = dirUrl.toString();
    QRegExp regex("^/([^/]+)$");
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    // send request
    QUrl form;
    form.addQueryItem("album[name]", regex.cap(1));

    QNetworkRequest req(FileInfo::fileUrl(baseUrl, "albums"));
    req.setRawHeader("Accept", "application/xml");
    job->baseUrl = baseVirtual;
    job->setNetworkReply(network->post(req, form.encodedQuery()));
    return job;
}

FileSystemJob* WifirstFileSystem::open(const QUrl &url)
{
    FileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Open, this);

    baseUrl = "https://www.wifirst.net/w";
    baseVirtual = url.toString();

    // clear caches
    albumHash.clear();
    pictureHash.clear();

    QNetworkRequest req(FileInfo::fileUrl(baseUrl, "albums"));
    req.setRawHeader("Accept", "application/xml");
    job->setNetworkReply(network->get(req));
    return job;
}

FileSystemJob* WifirstFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    WifirstFileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Put, this);

    // parse url
    int albumId = 0;
    QRegExp regex("^/([^/]+)/([^/]+)$");
    const QString urlString = fileUrl.toString();
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())) || !(albumId = albumHash[regex.cap(1)]))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    // send request
    MimeForm form;
    form.addString("picture[album_id]", QString::number(albumId));
    form.addFile("picture[data]", regex.cap(2), data->readAll());

    QNetworkRequest req(FileInfo::fileUrl(baseUrl, "pictures"));
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("Content-Type", "multipart/form-data; boundary=" + form.boundary);
    job->baseUrl = FileInfo::fileUrl(baseVirtual, regex.cap(1));
    job->setNetworkReply(network->post(req, form.render()));
    return job;
}

FileSystemJob* WifirstFileSystem::remove(const QUrl &fileUrl)
{
    WifirstFileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Remove, this);

    // parse url
    QRegExp regex("^/([^/]+)/([0-9]+).jpg$");
    const QString urlString = fileUrl.toString();
    int pictureRemoveId = 0;
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())) || !(pictureRemoveId = regex.cap(2).toInt()))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    // send request
    QUrl form;
    form.addQueryItem("target", "album_trash");
    form.addQueryItem("id[]", QString::number(pictureRemoveId));

    QNetworkRequest req(FileInfo::fileUrl(baseUrl, "pictures/drop"));
    req.setRawHeader("Accept", "application/xml");
    job->baseUrl = FileInfo::fileUrl(baseVirtual, regex.cap(1));
    job->setProperty("__picture_id", pictureRemoveId);
    job->setNetworkReply(network->post(req, form.encodedQuery()));
    return job;
}

FileSystemJob* WifirstFileSystem::rmdir(const QUrl &dirUrl)
{
    WifirstFileSystemJob *job = new WifirstFileSystemJob(FileSystemJob::Rmdir, this);

    // parse url
    int albumId = 0;
    QRegExp regex("^/([^/]+)$");
    const QString urlString = dirUrl.toString();
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())) || !(albumId = albumHash[regex.cap(1)]))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    // send request
    QNetworkRequest req(FileInfo::fileUrl(baseUrl, QString("albums/%1").arg(albumId)));
    req.setRawHeader("Accept", "application/xml");
    job->baseUrl = baseVirtual;
    job->setProperty("__album_name", regex.cap(1));
    job->setNetworkReply(network->deleteResource(req));
    return job;
}

FileInfo WifirstFileSystem::parseAlbum(const QDomElement &album)
{
    int id = album.firstChildElement("id").text().toInt();
    QString name = album.firstChildElement("name").text();

    FileInfo item;
    item.setUrl(FileInfo::fileUrl(baseVirtual, name));
    item.setName(name);
    item.setDir(true);
    QDateTime lastmodified = parseDateTime(album.firstChildElement("created-at").text());

    if (lastmodified.isValid())
        item.setLastModified(lastmodified);

    /* add to cache */
    albumHash[name] = id;
    return item;
}

FileInfoList WifirstFileSystem::parseFiles(const QDomElement &element, QString listBase)
{
    FileInfoList pictureList;

    QDomElement picture = element.firstChildElement("picture");
    while (!picture.isNull())
    {
        int id = picture.firstChildElement("id").text().toInt();
        QString url = picture.firstChildElement("url").text();

        QString name = QString::number(id) + ".jpg";
        pictureHash[id] = url;

        FileInfo item;
        item.setUrl(FileInfo::fileUrl(listBase, name));
        item.setName(name);
        item.setDir(false);
        item.setLastModified(parseDateTime(picture.firstChildElement("created-at").text()));
        item.setSize(picture.firstChildElement("size").text().toInt());
        pictureList.append(item);

        picture = picture.nextSiblingElement("picture");
    }

    return pictureList;
}

class WifirstFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("wifirst"))
            return new WifirstFileSystem(parent);
        return NULL;
    };
};

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_STATIC_PLUGIN2(wifirst_filesystem, WifirstFileSystemPlugin)
#endif
