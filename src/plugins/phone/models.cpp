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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "models.h"

PhoneCallsModel::PhoneCallsModel(QNetworkAccessManager *network, QObject *parent)
    : QAbstractListModel(parent),
    m_network(network)
{
}

QVariant PhoneCallsModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

void PhoneCallsModel::handleList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Failed to retrieve phone calls: %s", qPrintable(reply->errorString()));
        return;
    }

    qDebug("Got calls: %s", reply->readAll().constData());
}

int PhoneCallsModel::rowCount(const QModelIndex& parent) const
{
    return m_calls.size();
}

void PhoneCallsModel::setUrl(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    QNetworkReply *reply = m_network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(handleList()));
}

