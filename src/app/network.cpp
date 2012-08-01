/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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
#include <QDesktopServices>
#include <QDir>
#include <QLocale>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QSslError>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "network.h"

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    bool check;
    Q_UNUSED(check);

    const QString cachePath = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).filePath("cache");
    QDir().mkpath(cachePath);

    QNetworkDiskCache *cache = new QNetworkDiskCache(this);
    cache->setCacheDirectory(cachePath);
    setCache(cache);

    check = connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                    this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    Q_ASSERT(check);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request(req);
    if (!request.hasRawHeader("Accept-Language"))
        request.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    if (!request.hasRawHeader("User-Agent"))
        request.setRawHeader("User-Agent", userAgent().toAscii());
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

void NetworkAccessManager::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply);

    qWarning("SSL errors:");
    foreach (const QSslError &error, errors)
        qWarning("* %s", qPrintable(error.errorString()));

    // uncomment for debugging purposes only
    //reply->ignoreSslErrors();
}

QString NetworkAccessManager::userAgent()
{
    static QString globalUserAgent;

    if (globalUserAgent.isEmpty()) {
        QString osDetails;
#if defined(Q_OS_ANDROID)
        osDetails = QLatin1String("Linux; Android");
#elif defined(Q_OS_LINUX)
        osDetails = QLatin1String("X11; Linux");
        QProcess process;
        process.start(QString("uname"), QStringList(QString("-m")), QIODevice::ReadOnly);
        process.waitForFinished();
        const QString arch = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        if (!arch.isEmpty())
            osDetails += " " + arch;
#elif defined(Q_OS_MAC)
        osDetails = QLatin1String("Macintosh");
        switch (QSysInfo::MacintoshVersion)
        {
        case QSysInfo::MV_10_4:
            osDetails += QLatin1String("; Mac OS X 10.4");
            break;
        case QSysInfo::MV_10_5:
            osDetails += QLatin1String("; Mac OS X 10.5");
            break;
        case QSysInfo::MV_10_6:
            osDetails += QLatin1String("; Mac OS X 10.6");
            break;
        }
#elif defined(Q_OS_SYMBIAN)
        osDetails = QLatin1String("Symbian");
#elif defined(Q_OS_WIN)
        DWORD dwVersion = GetVersion();
        DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
        DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
        osDetails = QString("Windows NT %1.%2").arg(QString::number(dwMajorVersion), QString::number(dwMinorVersion));
#endif
        globalUserAgent = QString("Mozilla/5.0 (%1) %2/%3").arg(osDetails, qApp->applicationName(), qApp->applicationVersion());
    }
    return globalUserAgent;
}

