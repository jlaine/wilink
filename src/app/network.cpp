/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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
#include <QDir>
#include <QLocale>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QSslError>
#include <QStandardPaths>
#include <QSysInfo>

#include "network.h"

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    bool check;
    Q_UNUSED(check);

    check = connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                    this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    Q_ASSERT(check);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request(req);
    if (!request.hasRawHeader("Accept-Language"))
        request.setRawHeader("Accept-Language", QLocale::system().name().toLatin1());
    if (!request.hasRawHeader("User-Agent"))
        request.setRawHeader("User-Agent", userAgent().toLatin1());
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
        osDetails = QString("X11; Linux %1").arg(QSysInfo::currentCpuArchitecture());
#elif defined(Q_OS_MAC)
        osDetails = QString("Macintosh; Mac OS X %1").arg(QSysInfo::productVersion());
#elif defined(Q_OS_WIN)
        osDetails = QString("Windows NT %1").arg(QSysInfo::productVersion());
#endif
        globalUserAgent = QString("Mozilla/5.0 (%1) %2/%3").arg(osDetails, qApp->applicationName(), qApp->applicationVersion());
    }
    return globalUserAgent;
}

NetworkAccessManagerFactory::NetworkAccessManagerFactory()
{
    const QString dataPath = QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0];
    m_cachePath = QDir(dataPath).filePath("cache");
    QDir().mkpath(m_cachePath);
}

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject * parent)
{
    QNetworkAccessManager *manager = new NetworkAccessManager(parent);
    QNetworkDiskCache *cache = new QNetworkDiskCache(manager);
    cache->setCacheDirectory(m_cachePath);
    manager->setCache(cache);
    return manager;
}
