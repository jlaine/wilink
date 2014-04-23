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
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QUrl>

#include "http.h"
#include "wallet.h"

static QUrl addSlash(const QUrl &url)
{
    QUrl ret(url);
    if (!ret.path().endsWith("/"))
        ret.setPath(ret.path() + "/");
    return ret;
}

class HttpFileSystemJob : public FileSystemJob
{
public:
    HttpFileSystemJob(Operation op, HttpFileSystem *fs)
        : FileSystemJob(op, fs), m_fs(fs)
    {
    }

protected:
    void networkFinished(QNetworkReply *reply);

private:
    HttpFileSystem *m_fs;
};

void HttpFileSystemJob::networkFinished(QNetworkReply *reply)
{
    switch (operation())
    {
    case List:
        if (reply->error() == QNetworkReply::NoError)
            setResults(m_fs->davEnabled ? m_fs->parseDavList(reply->readAll()) : m_fs->parseHttpList(reply->readAll()));
        break;
    case Open:
        if (reply->error() == QNetworkReply::NoError)
            m_fs->davEnabled = reply->hasRawHeader("DAV");
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

HttpFileSystem::HttpFileSystem(QObject *parent)
    : FileSystem(parent),
    davEnabled(false)
{
    network = createNetworkAccessManager(this);
}

FileSystemJob* HttpFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(type);
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Get, this);
    QNetworkReply *reply = network->get(QNetworkRequest(fileUrl));
    job->setData(reply);
    job->setNetworkReply(reply);
    return job;
}

FileSystemJob* HttpFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(filter);
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::List, this);

    // append trailing slash if necessary
    listUrl = addSlash(dirUrl);

    // send request
    QNetworkRequest req(listUrl);
    if (davEnabled) {
        req.setRawHeader("Content-Type", "application/xml");
        req.setRawHeader("Depth", "1");

        QBuffer *buffer = new QBuffer;
        buffer->setData("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<propfind xmlns=\"DAV:\"><prop>"
        "<getcontentlength xmlns=\"DAV:\"/>"
        "<getlastmodified xmlns=\"DAV:\"/>"
        "<executable xmlns=\"http://apache.org/dav/props/\"/>"
        "<resourcetype xmlns=\"DAV:\"/>"
        "<checked-in xmlns=\"DAV:\"/>"
        "<checked-out xmlns=\"DAV:\"/>"
        "</prop></propfind>");
        buffer->open(QIODevice::ReadOnly);

        QNetworkReply *reply = network->sendCustomRequest(req, "PROPFIND", buffer);
        buffer->setParent(reply);
        job->setNetworkReply(reply);
    } else {
        job->setNetworkReply(network->get(req));
    }

    return job;
}

FileSystemJob* HttpFileSystem::mkdir(const QUrl &dirUrl)
{
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Mkdir, this);

    if (!davEnabled) {
        job->finishLater(FileSystemJob::UnknownError);
        return job;
    }
 
    job->setNetworkReply(network->sendCustomRequest(QNetworkRequest(dirUrl), "MKCOL", 0));
    return job;
}

FileSystemJob* HttpFileSystem::open(const QUrl &url)
{
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Open, this);
    job->setNetworkReply(network->sendCustomRequest(QNetworkRequest(url), "OPTIONS", 0));
    return job;
}

FileSystemJob* HttpFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Put, this);

    if (!davEnabled) {
        job->finishLater(FileSystemJob::UnknownError);
        return job;
    }

    job->setNetworkReply(network->put(QNetworkRequest(fileUrl), data));
    return job;
}

FileSystemJob* HttpFileSystem::remove(const QUrl &fileUrl)
{
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Remove, this);
    if (!davEnabled) {
        job->finishLater(FileSystemJob::UnknownError);
        return job;
    }

    job->setNetworkReply(network->deleteResource(QNetworkRequest(fileUrl)));
    return job;
}

FileSystemJob* HttpFileSystem::rmdir(const QUrl &dirUrl)
{
    FileSystemJob *job = new HttpFileSystemJob(FileSystemJob::Rmdir, this);
    if (!davEnabled) {
        job->finishLater(FileSystemJob::UnknownError);
        return job;
    }

    // FIXME: this should fail if the directory is not empty!
    job->setNetworkReply(network->deleteResource(QNetworkRequest(addSlash(dirUrl))));
    return job;
}

/** Parse a WebDAV PROPFIND response.
 *
 * @param data
 */
FileInfoList HttpFileSystem::parseDavList(const QByteArray &data)
{
    FileInfoList listItems;
    QDomDocument doc;
    doc.setContent(data);
    QString listPath = listUrl.path();

    QDomElement response = doc.documentElement().firstChildElement("D:response");
    while (!response.isNull())
    {
        QString href = QUrl::fromPercentEncoding(response.firstChildElement("D:href").text().toUtf8());

        if (href == listPath || !href.startsWith(listPath))
        {
            response = response.nextSiblingElement("D:response");
            continue;
        }

        /* get name */
        QString name = href.mid(listPath.length());
        bool isdir = false;
        if (name.endsWith("/"))
        {
            isdir = true;
            name.chop(1);
        }

        /* get properties */
        int size = 0;
        QDateTime lastmodified;
        QDomElement propstat = response.firstChildElement("D:propstat");
        while (!propstat.isNull())
        {
            QString status = propstat.firstChildElement("D:status").text();
            if (status.startsWith("HTTP/1.1 200"))
            {
                QDomElement prop = propstat.firstChildElement("D:prop").firstChildElement();
                while (!prop.isNull())
                {
                    //qDebug() << "prop" << prop.tagName() << prop.text();
                    if (prop.tagName() == "lp1:getcontentlength")
                    {
                        size = prop.text().toInt();
                    } else if (prop.tagName() == "lp1:getlastmodified") {
                        // example : "Wed, 24 Dec 2008 15:13:14 GMT"
                        QString date = prop.text().remove(QRegExp(" GMT$"));
                        lastmodified = QLocale::c().toDateTime(date, "ddd, dd MMM yyyy hh:mm:ss");
                        lastmodified.setTimeSpec(Qt::UTC);
                    }
                    prop = prop.nextSiblingElement();
                }
            }
            propstat = propstat.nextSiblingElement("D:propstat");
        }

        QUrl child(listUrl);
        child.setPath(href);

        FileInfo item;
        item.setUrl(child.toString());
        item.setName(name);
        item.setDir(isdir);
        if (lastmodified.isValid())
            item.setLastModified(lastmodified);
        item.setSize(size);
        listItems.append(item);

        response = response.nextSiblingElement("D:response");
    }

    return listItems;
}

/** Parse Apache directory index.
 *
 * @param data
 */
FileInfoList HttpFileSystem::parseHttpList(const QByteArray &data)
{
    FileInfoList listItems;
    QStringList lines = QString::fromUtf8(data).split("\n");

    // <IMG SRC="/icons/image2.gif" ALT="[IMG]"> <A HREF="kif_1626.jpg">kif_1626.jpg</A>            18-Oct-2007 00:03   349k
    // <tr><td valign="top"><img src="/icons/image2.gif" alt="[IMG]"></td><td><a href="blank.png">blank.png</a></td><td align="right">01-Dec-2008 11:04  </td><td align="right">189 </td></tr>
    QRegExp re(".*"
        "<img src=\"[^\"]+\" alt=\"([^\"]+)\">.*"
        "<a href=\"([^\"]+)\">([^<]+)</a>" // link and name
        ".*"
        "([0-9]{2}-[^\\s]{3}-[0-9]{4} [0-9]{2}:[0-9]{2})" // date
        ".*"
        "(-|([0-9\\.]+)([km])?).*" // size and unit
        ".*", Qt::CaseInsensitive);

    enum caps {
        ALL,
        IMG_ALT,
        LINK,
        NAME,
        DATE,
        SIZE,
        SIZE_FLOAT,
        SIZE_UNIT,
    };
    for (int i = 0; i < lines.size(); i++)
    {
        if (re.exactMatch(lines[i]))
        {
            FileInfo item;

            /* get name */
            QString name = re.cap(NAME);
            if (name.endsWith("/"))
                name.chop(1);
            item.setName(name);

            /* get url */
            QString url = re.cap(LINK);
            if (url.startsWith("/"))
                continue;
            item.setUrl(listUrl.resolved(url));

            /* parse type */
            if (re.cap(IMG_ALT).toLower() == "[dir]")
                item.setDir(true);

            /* parse date */
            QDateTime lastmodified = QLocale::c().toDateTime(re.cap(DATE), "dd-MMM-yyyy hh:mm");
            if (lastmodified.isValid())
                item.setLastModified(lastmodified);

            /* parse size */
            if (re.cap(SIZE) != "-")
            {
                float size = re.cap(SIZE_FLOAT).toFloat();
                QString unit = re.cap(SIZE_UNIT);
                if (unit.toLower() == "k")
                    size *= 1024;
                else if (unit.toLower() == "m")
                    size *= 1024 * 1024;
                else if (!unit.isEmpty())
                    qWarning() << "Unhandled size unit" << unit;
                item.setSize(size);
            }
            listItems.append(item);
        }
    }
    return listItems;
}

class HttpFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
            return new HttpFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(http_filesystem, HttpFileSystemPlugin)
