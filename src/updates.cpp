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
#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "updates.h"

Updates::Updates(QObject *parent)
    : QObject(parent)
{
    network = new QNetworkAccessManager(this);
    connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
}

void Updates::check(const QUrl &url, const QString &version)
{
    currentVersion = version;
    statusUrl = url;

    QNetworkRequest req(statusUrl);
    req.setRawHeader("Accept", "application/xml");
    network->get(req);
}

void Updates::requestFinished(QNetworkReply *reply)
{
    QDomDocument doc;

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Failed to check for updates");
        return;
    }

    doc.setContent(reply);
    QDomElement item = doc.documentElement();

    Release release;
    release.description = item.firstChildElement("description").text();
    release.url = QUrl(item.firstChildElement("url").text());
    release.version = item.firstChildElement("version").text();

    if (release.version != currentVersion)
        emit updateAvailable(release);
}

