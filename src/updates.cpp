/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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
    if (!updatesUrl.isValid())
        return;
    if (updatesUrl.scheme() != "https")
    {
        qWarning() << "Refusing to check for updates at insecure address" << updatesUrl;
        return;
    }

    QUrl statusUrl = updatesUrl;
    QList< QPair<QString, QString> > query = statusUrl.queryItems();
    query.append(qMakePair(QString::fromLatin1("ostype"), SystemInfo::osType()));
    query.append(qMakePair(QString::fromLatin1("osversion"), SystemInfo::osVersion()));
    query.append(qMakePair(QString::fromLatin1("version"), currentVersion));
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
    downloadFile.setFileName(QDir(dirPath).filePath(QFileInfo(release.url.path()).fileName()));

    qDebug() << "Downloading to" << downloadFile.fileName();

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
        qWarning("Failed to download update");
        emit updateFailed();
        return;
    }

    /* save file */
    if (!downloadFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write to" << downloadFile.fileName();
        emit updateFailed();
        return;
    }
    downloadFile.write(reply->readAll());
    downloadFile.close();

    emit updateDownloaded(QUrl::fromLocalFile(downloadFile.fileName()));
}

void Updates::processStatus()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Failed to check for updates");
        return;
    }

    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement();

    Release release;
    release.changes = item.firstChildElement("changes").text();
    release.package = item.firstChildElement("package").text();
    release.version = item.firstChildElement("version").text();
    const QString urlString = item.firstChildElement("url").text();
    if (!urlString.isEmpty())
        release.url = updatesUrl.resolved(QUrl(urlString));

    if (compareVersions(release.version, currentVersion) > 0 && !release.url.isEmpty())
        emit updateAvailable(release);
}

void Updates::setUrl(const QUrl &url)
{
    updatesUrl = url;
}

void Updates::setVersion(const QString &version)
{
    currentVersion = version;
}

