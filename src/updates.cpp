/*
 * wiLink
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
#include <QDesktopServices>
#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "systeminfo.h"
#include "updates.h"

/** Checks whether the given data matches the hashes specified
 *  in the release.
 *
 * @param data
 */
bool Release::checkHashes(const QByteArray &data) const
{
    foreach (const QString &type, hashes.keys())
    {
        if (type == "sha1")
        {
            QCryptographicHash hash(QCryptographicHash::Sha1);
            hash.addData(data);
            if (hash.result() != hashes[type])
                return false;
        }
    }
    return true;
}

/** Returns true if the release is a valid update from the
 *  currently installed version.
 */
bool Release::isValid() const
{
    return (Updates::compareVersions(version, qApp->applicationVersion()) > 0) &&
           url.isValid() && url.scheme() == "https" && hashes.contains("sha1");
}

Updates::Updates(QObject *parent)
    : QObject(parent),
    m_updatesUrl("https://download.wifirst.net/wiLink/")
{
    network = new QNetworkAccessManager(this);

    /* schedule updates */
    timer = new QTimer(this);
    timer->setInterval(500);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(check()));
    timer->start();
}

/** Returns the location where downloaded updates will be stored.
 */
QString Updates::cacheDirectory() const
{
    return m_cacheDirectory;
}

/** Sets the location where downloaded updates will be stored.
 *
 * @param cacheDir
 */
void Updates::setCacheDirectory(const QString &cacheDir)
{
    m_cacheDirectory = cacheDir;
}

/** Requests the current release information from the updates server.
 */
void Updates::check()
{
    emit checkStarted();

    /* only download files over HTTPS */
    if (m_updatesUrl.scheme() != "https")
    {
        emit checkFinished(Release(), "Refusing to check for updates from non-HTTPS site");
        return;
    }

    QUrl statusUrl = m_updatesUrl;
    QList< QPair<QString, QString> > query = statusUrl.queryItems();
    query.append(qMakePair(QString::fromLatin1("ostype"), SystemInfo::osType()));
    query.append(qMakePair(QString::fromLatin1("osversion"), SystemInfo::osVersion()));
    query.append(qMakePair(QString::fromLatin1("version"), qApp->applicationVersion()));
    statusUrl.setQueryItems(query);

    QNetworkRequest req(statusUrl);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
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

/** Downloads the specified release to the given file.
 */
void Updates::download(const Release &release)
{
    /* only download files over HTTPS with an SHA1 hash */
    if (!release.isValid())
    {
        emit updateFailed(InsecureLocation, "Refusing to download update from non-HTTPS site or without checksum");
        return;
    }

    downloadFile.setFileName(QDir(m_cacheDirectory).filePath(QFileInfo(release.url.path()).fileName()));

    /* check existing file */
    if (downloadFile.exists())
    {
        if (!downloadFile.open(QIODevice::ReadOnly))
        {
            emit updateFailed(SaveFailed, "Could not read downloaded file from disk");
            return;
        }

        /* if the hashes match, we are done here */
        const QByteArray data = downloadFile.readAll();
        downloadFile.close();
        if (release.checkHashes(data))
        {
            emit updateDownloaded(QUrl::fromLocalFile(downloadFile.fileName()));
            return;
        }
    }

    /* download release */
    downloadRelease = release;
    QNetworkRequest req(release.url);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(updateProgress(qint64, qint64)));
    connect(reply, SIGNAL(finished()), this, SLOT(saveUpdate()));
}

void Updates::install(const QUrl &url)
{
#ifdef Q_OS_WIN
    // invoke the downloaded installer on the same path as the current install
    QDir installDir(qApp->applicationDirPath());
    installDir.cdUp();

    // we cannot use QProcess::startDetached() because NSIS wants the
    // /D=.. argument to be absolutely unescaped.
    QString args = QString("\"%1\" /S /D=%2")
        .arg(url.toLocalFile().replace(QLatin1Char('/'), QLatin1Char('\\')))
        .arg(installDir.absolutePath().replace(QLatin1Char('/'), QLatin1Char('\\')));

    STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                               };
    PROCESS_INFORMATION pinfo;
    bool success = CreateProcessW(0, (wchar_t*)args.utf16(),
                                 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE, 0,
                                 0,
                                 &startupInfo, &pinfo);

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);

        // quit application to allow installation
        qApp->quit();
        return;
    }
#endif
    // open the downloaded archive
    QDesktopServices::openUrl(url);

    // quit application to allow installation
    qApp->quit();
}

/** Once a release has been downloaded, verify its checksum and write it
 *  to disk.
 */
void Updates::saveUpdate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        emit updateFailed(DownloadFailed, reply->errorString());
        return;
    }

    /* check data */
    const QByteArray data = reply->readAll();
    if (!downloadRelease.checkHashes(data))
    {
        emit updateFailed(BadHash, "The checksum of the downloaded file is incorrect");
        return;
    }

    /* save file */
    if (!downloadFile.open(QIODevice::WriteOnly))
    {
        emit updateFailed(SaveFailed, "Could not save downloaded file to disk");
        return;
    }
    downloadFile.write(data);
    downloadFile.close();

    emit updateDownloaded(QUrl::fromLocalFile(downloadFile.fileName()));
}

/** Handle a response from the updates server containing the current
 *  release information.
 */
void Updates::processStatus()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        /* retry in 5mn */
        timer->setInterval(300 * 1000);
        timer->start();
        emit checkFinished(Release(), reply->errorString());
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
        release.url = m_updatesUrl.resolved(QUrl(urlString));

    /* emit information about available release */
    emit checkFinished(release, QString());

    /* check again in two days */
    timer->setInterval(2 * 24 * 3600 * 1000);
    timer->start();
}

