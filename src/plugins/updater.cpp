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
    UpdaterPrivate(Updater *qq);
    QString cacheFile() const;
    bool checkCachedFile() const;
    void fail(Updater::Error newError, const QString &newString);

    Updater::Error error;
    QString errorString;
    QNetworkAccessManager *network;
    int progressValue;
    Release release;
    Updater::State state;
    QTimer *timer;
    QUrl updatesUrl;

private:
    Updater *q;
};

UpdaterPrivate::UpdaterPrivate(Updater *qq)
    : error(Updater::NoError),
    progressValue(0),
    state(Updater::IdleState),
    updatesUrl("https://download.wifirst.net/wiLink/"),
    q(qq)
{
    network = new NetworkAccessManager(q);
}

void UpdaterPrivate::fail(Updater::Error newError, const QString &newString)
{
    error = newError;
    errorString = newString;
    q->error(error, errorString);

    release = Release();
    q->updateChanged();

    state = Updater::IdleState;
    q->stateChanged(state);
}

QString UpdaterPrivate::cacheFile() const
{
    return QDir(wApp->settings()->downloadsLocation()).filePath(QFileInfo(release.url.path()).fileName());
}

bool UpdaterPrivate::checkCachedFile() const
{
    /* check file integrity */
    foreach (const QString &type, release.hashes.keys()) {
        if (type == QLatin1String("sha1")) {
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
    : QObject(parent)
{
    bool check;

    d = new UpdaterPrivate(this);

    // schedule updates
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    check = connect(d->timer, SIGNAL(timeout()),
                    this, SLOT(check()));
    Q_ASSERT(check);
    Q_UNUSED(check);

    // run initial check after 10s
    d->timer->start(10 * 1000);
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

    // only download files over HTTPS
    if (d->updatesUrl.scheme() != "https") {
        d->fail(SecurityError, "Refusing to check for updates from non-HTTPS site");
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

    // change state
    d->error = NoError;
    d->errorString = QString();
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

Updater::Error Updater::error() const
{
    return d->error;
}

QString Updater::errorString() const
{
    return d->errorString;
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
        d->fail(IntegrityError, "The checksum of the downloaded file is incorrect");
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
        .arg(d->cacheFile().replace(QLatin1Char('/'), QLatin1Char('\\')))
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

void Updater::refuse()
{
    if (d->state == PromptState) {
        d->error = NoError;
        d->errorString = QString();

        d->state = IdleState;
        emit stateChanged(d->state);
    }
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

    if (reply->error() != QNetworkReply::NoError) {
        // network error, retry in 5mn
        d->timer->start(5 * 60 * 1000);
        d->fail(NetworkError, reply->errorString());
        return;
    }

    QFile downloadFile(d->cacheFile());
    if (!downloadFile.open(QIODevice::WriteOnly)) {
        d->fail(FileError, "Could not save downloaded file to disk");
        return;
    }

    // save file
    char buffer[CHUNK_SIZE];
    qint64 length;
    while ((length = reply->read(buffer, CHUNK_SIZE)) > 0)
        downloadFile.write(buffer, length);
    downloadFile.close();

    // check integrity
    if (!d->checkCachedFile()) {
        downloadFile.remove();
        d->fail(IntegrityError, "The checksum of the downloaded file is incorrect");
        return;
    }

    // prompt user
    d->state = PromptState;
    emit stateChanged(d->state);
}

/** Handle a response from the updates server containing the current
 *  release information.
 */
void Updater::_q_processStatus()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError) {
        // network error, retry in 5mn
        d->timer->start(5 * 60 * 1000);
        d->fail(NetworkError, reply->errorString());
        return;
    }

    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement();

    // parse release information
    Release release;
    release.changes = item.firstChildElement("changes").text().trimmed();
    release.package = item.firstChildElement("package").text();
    release.version = item.firstChildElement("version").text();
    QDomElement hash = item.firstChildElement("hash");
    while (!hash.isNull()) {
        const QString type = hash.attribute("type");
        const QByteArray value = QByteArray::fromHex(hash.text().toAscii());
        if (!type.isEmpty() && !value.isEmpty())
            release.hashes[type] = value;
        hash = hash.nextSiblingElement("hash");
    }
    const QString urlString = item.firstChildElement("url").text();
    if (!urlString.isEmpty())
        release.url = d->updatesUrl.resolved(QUrl(urlString));

    if (Updater::compareVersions(release.version, qApp->applicationVersion()) <= 0 ||
        !release.url.isValid() ||
        release.url.scheme() != "https" ||
        !release.hashes.contains("sha1"))
    {
        // no update available, retry in 2 days
        d->timer->start(2 * 24 * 3600 * 1000);
        d->fail(NoUpdateError, "No update available.");
        return;
    }

    // found an update
    d->release = release;
    emit updateChanged();

    // check cached file
    if (d->checkCachedFile()) {
        d->state = PromptState;
        emit stateChanged(d->state);
        return;
    }

    // download release
    QNetworkRequest req(d->release.url);
    reply = d->network->get(req);

    bool check;
    Q_UNUSED(check);

    check = connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                    this, SLOT(_q_downloadProgress(qint64,qint64)));
    Q_ASSERT(check);

    check = connect(reply, SIGNAL(finished()),
                    this, SLOT(_q_saveUpdate()));
    Q_ASSERT(check);

    d->state = DownloadState;
    emit stateChanged(d->state);
}

