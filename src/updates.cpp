/*
 * wDesktop
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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>

#include "qnetio/wallet.h"

#include "systeminfo.h"
#include "updates.h"

Updates::Updates(QObject *parent)
    : QObject(parent)
{
    network = new QNetworkAccessManager(this);
    connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
}

void Updates::check()
{
    /* only download files over HTTPS */
    if (updatesUrl.scheme() != "https")
    {
        emit checkFailed(InsecureLocation);
        return;
    }

    QUrl statusUrl = updatesUrl;
    QList< QPair<QString, QString> > query = statusUrl.queryItems();
    query.append(qMakePair(QString::fromLatin1("ostype"), SystemInfo::osType()));
    query.append(qMakePair(QString::fromLatin1("osversion"), SystemInfo::osVersion()));
    query.append(qMakePair(QString::fromLatin1("version"), qApp->applicationVersion()));
    statusUrl.setQueryItems(query);

    QNetworkRequest req(statusUrl);
    req.setRawHeader("Accept", "application/xml");
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(processStatus()));
}

int Updates::compareVersions(const QString &v1, const QString v2)
{
    QStringList bits1 = v1.split(".");
    QStringList bits2 = v2.split(".");
    for (int i = 0; i < bits1.size(); ++i)
    {
        if (i >= bits2.size())
            return 1;
        int cmp = bits1.at(i).compare(bits2.at(i));
        if (cmp != 0)
            return cmp;
    }
    if (bits2.size() > bits1.size())
        return -1;
    return 0;
}

void Updates::download(const Release &release, const QString &dirPath)
{
    /* only download files over HTTPS with an SHA1 hash */
    if (release.url.scheme() != "https" || !release.hashes.contains("sha1"))
    {
        emit updateFailed(InsecureLocation);
        return;
    }

    downloadFile.setFileName(QDir(dirPath).filePath(QFileInfo(release.url.path()).fileName()));
    downloadRelease = release;

    QNetworkRequest req(release.url);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(updateProgress(qint64, qint64)));
    connect(reply, SIGNAL(finished()), this, SLOT(saveUpdate()));
}

void Updates::saveUpdate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        emit updateFailed(DownloadFailed);
        return;
    }

    /* check data */
    const QByteArray &data = reply->readAll();
    foreach (const QString &type, downloadRelease.hashes.keys())
    {
        if (type == "sha1")
        {
            QCryptographicHash hash(QCryptographicHash::Sha1);
            hash.addData(data);
            if (hash.result() != downloadRelease.hashes[type])
            {
                emit updateFailed(BadHash);
                return;
            } 
        }
    }

    /* save file */
    if (!downloadFile.open(QIODevice::WriteOnly))
    {
        emit updateFailed(SaveFailed);
        return;
    }
    downloadFile.write(data);
    downloadFile.close();

    emit updateDownloaded(QUrl::fromLocalFile(downloadFile.fileName()));
}

void Updates::processStatus()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed(DownloadFailed);
        return;
    }

    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement();

    /* parse release information */
    Release release;
    release.changes = item.firstChildElement("changes").text();
    release.package = item.firstChildElement("package").text();
    release.version = item.firstChildElement("version").text();
    QDomElement hash = item.firstChildElement("hash");
    while (!hash.isNull())
    {
        const QString type = hash.attribute("type");
        const QByteArray value = QByteArray::fromHex(hash.text().toAscii());
        if (!type.isEmpty() && !value.isEmpty())
            release.hashes[type] = value;
        hash = hash.nextSiblingElement("hash");
    }
    const QString urlString = item.firstChildElement("url").text();
    if (!urlString.isEmpty())
        release.url = updatesUrl.resolved(QUrl(urlString));

    /* check whether this version is more recent than the installed one */
    if (compareVersions(release.version, qApp->applicationVersion()) > 0 &&
        release.url.scheme() == "https" && release.hashes.contains("sha1"))
        emit updateAvailable(release);
}

void Updates::setUrl(const QUrl &url)
{
    updatesUrl = url;
}

