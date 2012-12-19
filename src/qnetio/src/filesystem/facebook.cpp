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
#include <QCryptographicHash>
#include <QDebug>
#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegExp>
#include <QUrl>

#include "facebook.h"
#include "mime.h"
#include "wallet.h"

#define API_KEY "a8e62672f39f6f631c992484d663f78f"
#define API_SECRET "8dafe846236c8407600c52d7c6fcba20"
#define API_SERVER "https://api.facebook.com/restserver.php"
#define API_VERSION "1.0"

/** Generate signatures compliant to http://wiki.developers.facebook.com/index.php/How_Facebook_Authenticates_Your_Application
 */
static QString generate_sig(QMap<QString,QString> &params, const QString &secret)
{
    QString tobehashed;
    foreach(QString key, params.keys()) {
        tobehashed.append(key).append("=").append(params[key]);
    }
    tobehashed.append(secret);
    return QCryptographicHash::hash(tobehashed.toUtf8(), QCryptographicHash::Md5).toHex();
}

FacebookFileSystem::FacebookFileSystem(QObject *parent)
    : FileSystem(parent), callId(1), secret(API_SECRET)
{
    bool check;

    network = createNetworkAccessManager(this);

    check = connect(network, SIGNAL(finished(QNetworkReply*)),
                    this, SLOT(onFinished(QNetworkReply*)));
    Q_ASSERT(check);
}

/** Perform a method call into the Facebook API
 */
void FacebookFileSystem::call(const QString &method, QMap<QString,QString> params)
{
    QUrl query;
    //currentMethod = method;
    params["method"] = method;
    params["api_key"] = API_KEY;
    params["v"] = API_VERSION;
    if (method != "facebook.auth.createToken")
    {
        params["session_key"] = sessionKey;
        params["call_id"] = QString::number(callId++);
    }

    foreach(QString key, params.keys())
        query.addQueryItem(key, params[key]);

    /* calculate signature */
    query.addQueryItem("sig", generate_sig(params, secret));

    QNetworkRequest req(QString(API_SERVER));
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    network->post(req, query.encodedQuery());
}

QIODevice *FacebookFileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    lastCommand = Get;

    QString realUrl = photoHash[fileUrl.toString()];
    if (realUrl.isEmpty()) {
        qWarning("Get was invoked on a bad URL");
        delayError(Get);
        return NULL;
    }

    return network->get(QNetworkRequest(realUrl));
}

void FacebookFileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    QMap<QString, QString> params;
    const QString urlString = dirUrl.toString();

    lastCommand = List;
    if (urlString == baseVirtual)
    {
        params["uid"] = uid;
        call("facebook.photos.getAlbums", params);
        return;
    }

    /* parse url */
    QRegExp regex("^/([^/]+)$");
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())))
    {
        qWarning("list could not parse url");
        emit commandFinished(List, true, FileInfoList());
        return;
    }
    QString albumId = albumHash[regex.cap(1)];

    /* send request */
    params["aid"] = albumId;
    call("facebook.photos.get", params);
}

FileSystemJob *FacebookFileSystem::mkdir(const QUrl &dirUrl)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Mkdir, this);

    /* parse URL */
    QRegExp regex("^/([^/]+)$");
    const QString urlString = dirUrl.toString();
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }

    /* send request */
    QMap<QString, QString> params;
    params["name"] = regex.cap(1);
    call("facebook.photos.createAlbum", params);
    return job;
}

FileSystemJob* FacebookFileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    FileSystemJob *job = new FileSystemJob(FileSystemJob::Put, this);

    QRegExp regex("^/([^/]+)/([^/\\.]+)[^/]*$");
    const QString urlString = fileUrl.toString();
    if (!urlString.startsWith(baseVirtual) || !regex.exactMatch(urlString.mid(baseVirtual.length())))
    {
        job->finishLater(FileSystemJob::UrlError);
        return job;
    }
    QString albumId = albumHash[regex.cap(1)];

    MimeForm form;
    QMap<QString,QString> params;
    params["method"] = "facebook.photos.upload";
    params["api_key"] = API_KEY;
    params["call_id"] = QString::number(callId++);
    params["session_key"] = sessionKey;
    params["caption"] = regex.cap(2);
    params["aid"] = albumId;
    params["v"] = API_VERSION;
    foreach(QString key, params.keys())
        form.addString(key, params[key]);
    form.addString("sig", generate_sig(params, secret));
    form.addFile("", regex.cap(2), data->readAll());

    QNetworkRequest req(QString(API_SERVER));
    req.setRawHeader("Content-Type", "multipart/form-data; boundary=" + form.boundary);
    job->setNetworkReply(network->post(req, form.render()));

    return job;
}

void FacebookFileSystem::open(const QUrl &url)
{
    baseVirtual = url.toString();
    lastCommand = Open;
    call("facebook.auth.createToken", QMap<QString, QString>());
}

FileInfoList FacebookFileSystem::handleList(QNetworkReply *reply)
{
    FileInfoList results;

    QDomDocument doc;
    doc.setContent(reply);

    /* parse albums */
    QDomElement album = doc.documentElement().firstChildElement("album");
    while (!album.isNull())
    {
        QString id = album.firstChildElement("aid").text();
        QString name = album.firstChildElement("name").text();

        FileInfo item;
        item.setDir(true);
        item.setName(name);
        item.setUrl(FileInfo::fileUrl(baseVirtual, name));

        /* add to cache */
        albumHash[name] = id;
        reversedAlbumHash[id] = name;
        results.append(item);

        album = album.nextSiblingElement("album");
    }

    /* parse photos */
    QDomElement photo = doc.documentElement().firstChildElement("photo");
    while (!photo.isNull())
    {
        QString albumId = photo.firstChildElement("aid").text();
        QString id = photo.firstChildElement("pid").text();
        QDomElement caption = photo.firstChildElement("caption");
        /* FIXME: handle case where name is not unique (e.g. several photos with same caption) */
        QString name = (caption.isNull() ? id  : caption.text()) + ".jpg";
        QString photoUrl = baseVirtual + "/" + reversedAlbumHash[albumId]  + "/" + name;

        FileInfo item;
        item.setDir(false);
        item.setName(name);
        item.setUrl(photoUrl);

        /* add to cache */
        results.append(item);
        photoHash[photoUrl] = photo.firstChildElement("src").text();
        photo = photo.nextSiblingElement("photo");
    }
    return results;
}

void FacebookFileSystem::handleOpen(QNetworkReply *reply)
{
    static int step = 0;

    /* handle error */
    if (reply->error())
    {
        qWarning("Login failed");
        emit commandFinished(Open, true, FileInfoList());
        return;
    }

    if (!authToken.size())
    {
        /* get auth token */
        QDomDocument result;
        result.setContent(reply);
        authToken = result.documentElement().text();

        /* get homepage */
        QNetworkRequest req(QString("http://www.facebook.com/"));
        network->get(req);
    } else if (step == 0) {
        step = 1;

        /* get credentials */
        QAuthenticator auth;
        Wallet::instance()->onAuthenticationRequired("www.facebook.com", &auth);
    
        /* login */
        QUrl query;
        query.addQueryItem("api_key", API_KEY);
        query.addQueryItem("auth_token", authToken);
        query.addQueryItem("email", auth.user());
        query.addQueryItem("pass", auth.password());
        query.addQueryItem("pass_placeHolder", "Password");
        query.addQueryItem("persistent", "1");
        query.addQueryItem("v", API_VERSION);

        QNetworkRequest req(QString("http://www.facebook.com/login.php"));
        req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        network->post(req, query.encodedQuery());
    } else if (step == 1) {
        step = 2;

        /* authorize photo upload */
        QUrl query;
        query.addQueryItem("api_key", API_KEY);
        query.addQueryItem("ext_perm", "photo_upload");
        query.addQueryItem("v", API_VERSION);

        QNetworkRequest req(QString("http://www.facebook.com/authorize.php"));
        req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        network->post(req, query.encodedQuery());

    } else if (step == 2) {
        step = 3;

        /* get session */
        QMap<QString,QString> params;
        params["auth_token"] = authToken;
        params["generate_session_secret"] = "True";
        call("facebook.auth.getSession", params);

    } else {
        /* parse session */
        QDomDocument result;
        result.setContent(reply);
        sessionKey = result.documentElement().firstChildElement("session_key").text();
        secret = result.documentElement().firstChildElement("secret").text();
        uid = result.documentElement().firstChildElement("uid").text();

        emit commandFinished(Open, false, FileInfoList());
    }
}

void FacebookFileSystem::onFinished(QNetworkReply *reply)
{
    FileInfoList results;
    bool error = reply->error();

    switch (lastCommand)
    {
    case Open:
        handleOpen(reply);
        return;
    case List:
        if (!error)
            results = handleList(reply);
        break;
    case Mkdir:
        if (!error)
            handleList(reply);
        break;
    }

    if (lastCommand != Get)
        reply->deleteLater();
    emit commandFinished(lastCommand, error, results);
}

class FacebookFileSystemPlugin : public QNetIO::FileSystemPlugin
{
public:
    QNetIO::FileSystem *create(const QUrl &url, QObject *parent) {
        if (url.scheme() == QLatin1String("facebook"))
            return new FacebookFileSystem(parent);
        return NULL;
    };
};

Q_EXPORT_STATIC_PLUGIN2(facebook_filesystem, FacebookFileSystemPlugin)
