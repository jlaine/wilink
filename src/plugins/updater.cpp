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
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "application.h"
#include "declarative.h"
#include "systeminfo.h"
#include "updater.h"

#define CHUNK_SIZE 16384
#define PROGRESS_MAX 100

class Release
{
public:
    QString changes;
    QString package;
    QUrl url;
    QString version;
    QMap<QString, QByteArray> hashes;
};

class UpdaterPrivate
{
public:
    QString cacheFile() const;
    bool checkCachedFile() const;

    Release release;
    QUrl updatesUrl;

    int progressValue;

    QNetworkAccessManager *network;
    Updater::State state;
    QTimer *timer;
};

QString UpdaterPrivate::cacheFile() const
{
    return QDir(wApp->downloadsLocation()).filePath(QFileInfo(release.url.path()).fileName());
}

bool UpdaterPrivate::checkCachedFile() const
{
    /* check file integrity */
    foreach (const QString &type, release.hashes.keys()) {
        if (type == "sha1") {
            QFile downloadFile(cacheFile());
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
Updater::Updater(QObject *parent)
    : QObject(parent),
    d(new UpdaterPrivate)
{
    bool check;

    d->progressValue = 0;
    d->updatesUrl = QUrl("https://download.wifirst.net/wiLink/");
    d->network = new NetworkAccessManager(this);
    d->state = IdleState;

    // schedule updates
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    check = connect(d->timer, SIGNAL(timeout()),
                    this, SLOT(check()));
    Q_ASSERT(check);
    Q_UNUSED(check);

    d->timer->start(500);
}

/** Destroys an updates checker.
 */
Updater::~Updater()
{
    delete d;
}

/** Requests the current release information from the updates server.
 */
void Updater::check()
{
    if (d->state != IdleState) {
        qWarning("Not starting check, operation in progress");
        return;
    }

    /* only download files over HTTPS */
    if (d->updatesUrl.scheme() != "https") {
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
    QNetworkReply *reply = d->network->get(req);
    connect(reply, SIGNAL(finished()),
            this, SLOT(_q_processStatus()));

    d->state = CheckState;
    emit stateChanged(d->state);
}

int Updater::compareVersions(const QString &v1, const QString v2)
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
void Updater::download()
{
    // check cached file
    if (d->checkCachedFile()) {
        d->state = PromptState;
        emit stateChanged(d->state);
        return;
    }

    // download release
    QNetworkRequest req(d->release.url);
    QNetworkReply *reply = d->network->get(req);

    bool check;
    check = connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
                    this, SLOT(_q_downloadProgress(qint64, qint64)));
    Q_ASSERT(check);

    check = connect(reply, SIGNAL(finished()),
                    this, SLOT(_q_saveUpdate()));
    Q_ASSERT(check);

    d->state = DownloadState;
    emit stateChanged(d->state);
}

Updater::State Updater::state() const
{
    return d->state;
}

void Updater::install()
{
    if (d->state != PromptState) {
        qWarning("Not starting install, operation in progress");
        return;
    }

    // check file integrity
    if (!d->checkCachedFile()) {
        emit error(IntegrityError, "The checksum of the downloaded file is incorrect");
        return;
    }

    // update state
    d->state = InstallState;
    emit stateChanged(d->state);

#if defined(Q_OS_SYMBIAN)
    // open the download directory in file browser
    QFileInfo fileInfo(d->cacheFile());
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().path()));
#elif defined(Q_OS_WIN)
    // invoke the downloaded installer on the same path as the current install
    QDir installDir(qApp->applicationDirPath());
    installDir.cdUp();

    // we cannot use QProcess::startDetached() because NSIS wants the
    // /D=.. argument to be absolutely unescaped.
    QString args = QString("\"%1\" /S /D=%2")
        .arg(d->cacheFile(d->release).replace(QLatin1Char('/'), QLatin1Char('\\')))
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
        QDesktopServices::openUrl(QUrl::fromLocalFile(d->cacheFile()));
    }

    // quit application to allow installation
    qApp->quit();
#else
    // open the downloaded archive
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->cacheFile()));

    // quit application to allow installation
    qApp->quit();
#endif
}

int Updater::progressMaximum() const
{
    return PROGRESS_MAX;
}

int Updater::progressValue() const
{
    return d->progressValue;
}

QString Updater::updateChanges() const
{
    return d->release.changes;
}

QString Updater::updateVersion() const
{
    return d->release.version;
}

void Updater::_q_downloadProgress(qint64 done, qint64 total)
{
    const int value = (total > 0) ? (done * PROGRESS_MAX) / total : 0;
    if (value != d->progressValue) {
        d->progressValue = value;
        emit progressValueChanged(d->progressValue);
    }
}

/** Once a release has been downloaded, verify its checksum and write it
 *  to disk.
 */
void Updater::_q_saveUpdate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() == QNetworkReply::NoError) {
        QFile downloadFile(d->cacheFile());
        if (downloadFile.open(QIODevice::WriteOnly)) {
            // save file
            char buffer[CHUNK_SIZE];
            qint64 length;
            while ((length = reply->read(buffer, CHUNK_SIZE)) > 0)
                downloadFile.write(buffer, length);
            downloadFile.close();

            // check integrity
            if (d->checkCachedFile()) {
                d->state = PromptState;
                emit stateChanged(d->state);
                return;
            } else {
                downloadFile.remove();
                emit error(IntegrityError, "The checksum of the downloaded file is incorrect");
            }
        } else {
            emit error(FileError, "Could not save downloaded file to disk");
        }
    } else {
        emit error(NetworkError, reply->errorString());
    }

    // update state
    d->release = Release();
    emit updateChanged();
    d->state = IdleState;
    emit stateChanged(d->state);
}

/** Handle a response from the updates server containing the current
 *  release information.
 */
void Updater::_q_processStatus()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() == QNetworkReply::NoError) {
        QDomDocument doc;
        doc.setContent(reply);
        QDomElement item = doc.documentElement();

        // parse release information
        Release release;
        release.changes = item.firstChildElement("changes").text().trimmed();
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

        if (Updater::compareVersions(release.version, qApp->applicationVersion()) > 0 &&
            release.url.isValid() &&
            release.url.scheme() == "https" &&
            release.hashes.contains("sha1"))
        {
            // download the file
            d->release = release;
            emit updateChanged();
            d->state = IdleState;
            download();
            return;
        } else {
            // no update available, check again in two days
            d->timer->start(2 * 24 * 3600 * 1000);
        }

    } else {
        // network error, retry in 5mn
        d->timer->start(5 * 60 * 1000);
        emit error(NetworkError, reply->errorString());
    }

    // update state
    d->state = IdleState;
    emit stateChanged(d->state);
}

