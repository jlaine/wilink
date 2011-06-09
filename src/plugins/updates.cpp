/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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
#include <QFile>
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

#define CHUNK_SIZE 16384

/** Returns the running application's release.
 */
Release Release::applicationRelease()
{
    Release release;
    release.package = qApp->applicationName();
    release.version = qApp->applicationVersion();
    return release;
}

/** Returns true if the release is a valid update from the
 *  currently installed version.
 */
bool Release::isValid() const
{
    return (Updates::compareVersions(version, qApp->applicationVersion()) > 0) &&
           url.isValid() && url.scheme() == "https" && hashes.contains("sha1");
}

class UpdatesPrivate
{
public:
    QString cacheFile(const Release &release) const;
    bool checkCachedFile(const Release &release) const;

    QString cacheDirectory;
    Release downloadRelease;
    QUrl updatesUrl;

    QNetworkAccessManager *network;
    QTimer *timer;
};

QString UpdatesPrivate::cacheFile(const Release &release) const
{
    return QDir(cacheDirectory).filePath(QFileInfo(release.url.path()).fileName());
}

bool UpdatesPrivate::checkCachedFile(const Release &release) const
{
    /* check file integrity */
    foreach (const QString &type, release.hashes.keys())
    {
        if (type == "sha1")
        {
            QFile downloadFile(cacheFile(release));
            if (!downloadFile.open(QIODevice::ReadOnly))
                return false;

            char buffer[CHUNK_SIZE];
            qint64 length;
            QCryptographicHash hash(QCryptographicHash::Sha1);
            while ((length = downloadFile.read(buffer, CHUNK_SIZE)) > 0)
                hash.addData(buffer, length);
            downloadFile.close();
            return hash.result() == release.hashes[type];
        }
    }

    return false;
}

/** Constructs a new updates checker.
 *
 * @param parent
 */
Updates::Updates(QObject *parent)
    : QObject(parent),
    d(new UpdatesPrivate)
{
    d->updatesUrl = QUrl("https://download.wifirst.net/wiLink/");
    d->network = new QNetworkAccessManager(this);

    /* schedule updates */
    d->timer = new QTimer(this);
    d->timer->setInterval(500);
    d->timer->setSingleShot(true);
    connect(d->timer, SIGNAL(timeout()), this, SLOT(check()));
    d->timer->start();
}

/** Destroys an updates checker.
 */
Updates::~Updates()
{
    delete d;
}

/** Returns the location where downloaded updates will be stored.
 */
QString Updates::cacheDirectory() const
{
    return d->cacheDirectory;
}

/** Sets the location where downloaded updates will be stored.
 *
 * @param cacheDir
 */
void Updates::setCacheDirectory(const QString &cacheDir)
{
    d->cacheDirectory = cacheDir;
}

/** Requests the current release information from the updates server.
 */
void Updates::check()
{
    if (isDownloading())
    {
        qWarning("Not checking for updates, download in progress");
        return;
    }

    emit checkStarted();

    /* only download files over HTTPS */
    if (d->updatesUrl.scheme() != "https")
    {
        emit error(SecurityError, "Refusing to check for updates from non-HTTPS site");
        return;
    }

    QUrl statusUrl = d->updatesUrl;
    QList< QPair<QString, QString> > query = statusUrl.queryItems();
    query.append(qMakePair(QString::fromLatin1("ostype"), SystemInfo::osType()));
    query.append(qMakePair(QString::fromLatin1("osversion"), SystemInfo::osVersion()));
    query.append(qMakePair(QString::fromLatin1("version"), qApp->applicationVersion()));
    statusUrl.setQueryItems(query);

    QNetworkRequest req(statusUrl);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    QNetworkReply *reply = d->network->get(req);
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
    if (isDownloading())
    {
        qWarning("Not starting download, download already in progress");
        return;
    }

    /* only download files over HTTPS with an SHA1 hash */
    if (!release.isValid())
    {
        emit error(SecurityError, "Refusing to download update from non-HTTPS site or without checksum");
        return;
    }

    /* check cached file */
    if (d->checkCachedFile(release))
    {
        emit downloadFinished(release);
        return;
    }

    /* download release */
    d->downloadRelease = release;
    QNetworkRequest req(release.url);
    QNetworkReply *reply = d->network->get(req);

    bool check;
    check = connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
                    this, SIGNAL(downloadProgress(qint64, qint64)));
    Q_ASSERT(check);

    check = connect(reply, SIGNAL(finished()),
                    this, SLOT(saveUpdate()));
    Q_ASSERT(check);
}

/** Returns true if an update is currently being downloaded.
 */
bool Updates::isDownloading() const
{
    return d->downloadRelease.isValid();
}

void Updates::install(const Release &release)
{
    /* check file integrity */
    if (!d->checkCachedFile(release))
    {
        emit error(IntegrityError, "The checksum of the downloaded file is incorrect");
        return;
    }

#if defined(Q_OS_SYMBIAN)
    // open the download directory in file browser
    QFileInfo fileInfo(d->cacheFile(release));
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().path()));
#elif defined(Q_OS_WIN)
    // invoke the downloaded installer on the same path as the current install
    QDir installDir(qApp->applicationDirPath());
    installDir.cdUp();

    // we cannot use QProcess::startDetached() because NSIS wants the
    // /D=.. argument to be absolutely unescaped.
    QString args = QString("\"%1\" /S /D=%2")
        .arg(d->cacheFile(release).replace(QLatin1Char('/'), QLatin1Char('\\')))
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

    // if we cannot run process, fallback to opening the file
    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(d->cacheFile(release)));
    }

    // quit application to allow installation
    qApp->quit();
#else
    // open the downloaded archive
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->cacheFile(release)));

    // quit application to allow installation
    qApp->quit();
#endif
}

/** Once a release has been downloaded, verify its checksum and write it
 *  to disk.
 */
void Updates::saveUpdate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    const Release release = d->downloadRelease;
    d->downloadRelease = Release();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit error(NetworkError, reply->errorString());
        return;
    }

    /* save file */
    QFile downloadFile(d->cacheFile(release));
    if (!downloadFile.open(QIODevice::WriteOnly))
    {
        emit error(FileError, "Could not save downloaded file to disk");
        return;
    }
    char buffer[CHUNK_SIZE];
    qint64 length;
    while ((length = reply->read(buffer, CHUNK_SIZE)) > 0)
        downloadFile.write(buffer, length);
    downloadFile.close();

    /* if integrity check fails, delete downloaded file */
    if (!d->checkCachedFile(release))
    {
        downloadFile.remove();
        emit error(IntegrityError, "The checksum of the downloaded file is incorrect");
        return;
    }
    emit downloadFinished(release);
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
        d->timer->setInterval(300 * 1000);
        d->timer->start();
        emit error(NetworkError, reply->errorString());
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
        release.url = d->updatesUrl.resolved(QUrl(urlString));

    /* emit information about available release */
    if (release.isValid())
        emit checkFinished(release);
    else
        emit checkFinished(Release());

    /* check again in two days */
    d->timer->setInterval(2 * 24 * 3600 * 1000);
    d->timer->start();
}

