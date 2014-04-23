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

#include <QAuthenticator>
#include <QDebug>
#include <QDomDocument>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>

#include "picasa.h"
#include "wallet.h"

void FileMapper::addPath(const QUrl &virtualUrl, const QUrl &realUrl)
{
    fileHash[virtualUrl.toString()] = realUrl;
}

QUrl FileMapper::path(const QUrl &url)
{
    return fileHash[url.toString()];
}

QPair<QUrl, QString> FileMapper::splitPath(const QUrl &url)
{
    QRegExp regex("(.*)/([^/]+)$");
    if (!regex.exactMatch(url.toString()))
        return qMakePair(QUrl(), QString());

    return qMakePair(QUrl(regex.cap(1)), regex.cap(2));
}

class PicasaFileSystemJob : public FileSystemJob
{
public:
    PicasaFileSystemJob(Operation op, PicasaFileSystem *fs)
        : FileSystemJob(op, fs), m_fs(fs)
    {
    }

    QUrl baseUrl;

protected:
    void networkFinished(QNetworkReply *reply);

private:
    PicasaFileSystem *m_fs;
};

void PicasaFileSystemJob::networkFinished(QNetworkReply *reply)
{
    switch (operation())
    {
    case List:
        if (reply->error() == QNetworkReply::NoError) {
            QDomDocument doc;
            doc.setContent(reply);

            FileInfoList results;
            QDomElement entry = doc.documentElement().firstChildElement("entry");
            while (!entry.isNull()) {
                results.append(m_fs->parseEntry(entry, baseUrl));
                entry = entry.nextSiblingElement("entry");
            }
            setAllowedOperations(List | Mkdir | Rmdir | Put);
            setResults(results);
        }
        break;
    case Mkdir:
        if (reply->error() == QNetworkReply::NoError) {
            QDomDocument doc;
            doc.setContent(reply);

            QDomElement entry = doc.documentElement();
            m_fs->parseEntry(entry, baseUrl);
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    case Open:
        if (reply->error() == QNetworkReply::NoError) {
            /* parse ClientLogin reply */
            QString line;
            QHash<QString, QString> tokens;
            while (!(line = reply->readLine()).isEmpty())
            {
                QStringList bits = line.trimmed().split("=");
                tokens[bits[0]] = bits[1];
            }
            if (tokens["Auth"].isEmpty())
            {
                qWarning("ClientLogin does not contain Auth");
            } else {
                m_fs->authToken = tokens["Auth"];
            }
        }
        break;
    case Put:
        if (reply->error() == QNetworkReply::NoError) {
            emit m_fs->directoryChanged(baseUrl);
        }
        break;
    case Remove:
    case Rmdir:
        if (reply->error() == QNetworkReply::NoError) {
            emit m_fs->directoryChanged(baseUrl);
        } else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 409) {
            // If delete failed with code 409, try again
            QDomDocument doc;
            doc.setContent(reply);

            QDomElement lnk = doc.documentElement().firstChildElement("link");
            while (!lnk.isNull())
            {
                if (lnk.attribute("rel") == "edit")
                {
                    QNetworkRequest req(lnk.attribute("href"));
                    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(m_fs->authToken).toUtf8());
                    setNetworkReply(m_fs->network->deleteResource(req));
                    return;
                }
                lnk = lnk.nextSiblingElement("link");
            }
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

PicasaFileSystem::PicasaFileSystem(QObject *parent)
    : FileSystem(parent)
{
    network = createNetworkAccessManager(this);
}

FileSystemJob* PicasaFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    PicasaFileSystemJob *job = new PicasaFileSystemJob(FileSystemJob::Get, this);

    QString realUrl = fileHash[fileUrl.toString()];
    if (realUrl.isEmpty()) {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }
    if (type == SmallSize && smallHash.contains(fileUrl.toString()))
        realUrl = smallHash[fileUrl.toString()];
    else if (type == MediumSize && mediumHash.contains(fileUrl.toString()))
        realUrl = mediumHash[fileUrl.toString()];

    QNetworkRequest req(realUrl);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(authToken).toUtf8());

    QNetworkReply *reply = network->get(req);
    job->setData(reply);
    job->setNetworkReply(reply);
    return job;
}

FileSystemJob* PicasaFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(filter);
    PicasaFileSystemJob *job = new PicasaFileSystemJob(FileSystemJob::List, this);

    QUrl url = mapper.path(dirUrl);
    if (url.isEmpty()) {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(authToken).toUtf8());
    job->baseUrl = dirUrl;
    job->setNetworkReply(network->get(req));
    return job;
}

FileSystemJob* PicasaFileSystem::mkdir(const QUrl &dirUrl)
{
    PicasaFileSystemJob *job = new PicasaFileSystemJob(FileSystemJob::Mkdir, this);

    const QPair<QUrl, QString> bits = mapper.splitPath(dirUrl);
    const QUrl postUrl = mapper.path(bits.first);
    if (postUrl.isEmpty()) {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    QDomDocument doc;
    doc.setContent(QString(
"<entry xmlns='http://www.w3.org/2005/Atom'"
"    xmlns:media='http://search.yahoo.com/mrss/'"
"    xmlns:gphoto='http://schemas.google.com/photos/2007'>"
"  <category scheme='http://schemas.google.com/g/2005#kind'"
"    term='http://schemas.google.com/photos/2007#album'></category>"
"</entry>"));
    QDomElement entry = doc.documentElement();
    QDomElement item;

    item = doc.createElement("title");
    item.setAttribute("type", "text");
    item.appendChild(doc.createTextNode(bits.second));
    entry.appendChild(item);

#if 0
    item = doc.createElement("summary");
    item.setAttribute("type", "text");
    item.appendChild(doc.createTextNode("This was the recent trip I took to Italy."));
    entry.appendChild(item);

    QHash<QString, QString> items;
    items["gphoto:access"] = "public";
    items["gphoto:location"] = "Italy";
    items["gphoto:commentingEnabled"] = "true";
    items["gphoto:timestamp"] = "1152255600000";

    QHash<QString, QString>::const_iterator i = items.constBegin();
    while (i != items.constEnd()) {
        item = doc.createElement(i.key());
        item.appendChild(doc.createTextNode(i.value()));
        entry.appendChild(item);
        ++i;
    }
#endif

    QNetworkRequest req(postUrl);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(authToken).toUtf8());
    req.setRawHeader("Content-Type", "application/atom+xml; charset=UTF-8");
    job->baseUrl = bits.first;
    job->setNetworkReply(network->post(req, doc.toByteArray()));
    return job;
}

FileSystemJob* PicasaFileSystem::open(const QUrl &url)
{
    PicasaFileSystemJob *job = new PicasaFileSystemJob(FileSystemJob::Open, this);

    mapper.addPath(url, QUrl("http://picasaweb.google.com/data/feed/api/user/default"));

    QAuthenticator auth;
    Wallet::instance()->onAuthenticationRequired("www.google.com", &auth);

    QUrl query;
    query.addQueryItem("accountType", "HOSTED_OR_GOOGLE");
    query.addQueryItem("Email", auth.user());
    query.addQueryItem("Passwd", auth.password());
    query.addQueryItem("service", "lh2");
    query.addQueryItem("source", "BolloreTelecom-BoC-1.0");

    QNetworkRequest req(QString("https://www.google.com/accounts/ClientLogin"));
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    job->setNetworkReply(network->post(req, query.encodedQuery()));
    return job;
}

FileSystemJob* PicasaFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    PicasaFileSystemJob *job = new PicasaFileSystemJob(FileSystemJob::Put, this);

    const QPair<QUrl, QString> bits = mapper.splitPath(fileUrl);
    const QUrl postUrl = mapper.path(bits.first);
    if (postUrl.isEmpty()) {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    QNetworkRequest req(postUrl);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(authToken).toUtf8());
    req.setRawHeader("Content-Type", "image/jpeg");
    req.setRawHeader("Slug", bits.second.toUtf8());

    job->baseUrl = bits.first;
    job->setNetworkReply(network->post(req, data));
    return job;
}

FileSystemJob* PicasaFileSystem::remove(const QUrl &fileUrl)
{
    return removeEntry(FileSystemJob::Remove, fileUrl);
}

FileSystemJob* PicasaFileSystem::rmdir(const QUrl &dirUrl)
{
    return removeEntry(FileSystemJob::Rmdir, dirUrl);
}

FileInfo PicasaFileSystem::parseEntry(QDomElement &entry, const QUrl &baseUrl)
{
    QString name = entry.firstChildElement("title").text();
    QString link = entry.firstChildElement("link").attribute("href");
    QString id = entry.firstChildElement("gphoto:id").text();
    int size = entry.firstChildElement("gphoto:size").text().toInt();

    QUrl path = FileInfo::fileUrl(baseUrl, name);

    /* find links */
    QDomElement lnk = entry.firstChildElement("link");
    while (!lnk.isNull())
    {
        if (lnk.attribute("rel") == "edit")
            editHash[path.toString()] = lnk.attribute("href");
        lnk = lnk.nextSiblingElement("link");
    }

    FileInfo item;
    item.setName(name);
    item.setUrl(path);
    if (size)
    {
        item.setSize(size);
        fileHash[path.toString()] = entry.firstChildElement("content").attribute("src");
        QDomElement thumbnail = entry.firstChildElement("media:group").firstChildElement("media:thumbnail");
        while (!thumbnail.isNull())
        {
            // available sizes: 72, 144, 240
            if (thumbnail.attribute("width") == "144")
                smallHash[path.toString()] = thumbnail.attribute("url");
            if (thumbnail.attribute("width") == "240")
                mediumHash[path.toString()] = thumbnail.attribute("url");
            thumbnail = thumbnail.nextSiblingElement("media:thumbnail");
        }
    } else {
        item.setDir(true);
    }
    mapper.addPath(path, link);
    return item;
}

FileSystemJob *PicasaFileSystem::removeEntry(FileSystemJob::Operation operation, const QUrl &url)
{
    PicasaFileSystemJob *job = new PicasaFileSystemJob(operation, this);

    const QPair<QUrl, QString> bits = mapper.splitPath(url);
    const QString postUrl = editHash[url.toString()];
    if (postUrl.isEmpty())
    {
        qWarning("remove was invoked on a bad URL");
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    QNetworkRequest req(postUrl);
    req.setRawHeader("Authorization", QString("GoogleLogin auth=%1").arg(authToken).toUtf8());
    job->baseUrl = bits.first;
    job->setNetworkReply(network->deleteResource(req));
    return job;
}

class PicasaFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("picasa"))
            return new PicasaFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(picasa_filesystem, PicasaFileSystemPlugin)
