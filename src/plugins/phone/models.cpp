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
#include <QDomDocument>
#include <QDomElement>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "models.h"

class PhoneCallsItem
{
public:
    int id;
    QString address;
    QDateTime date;
    int duration;
    int flags;
};

PhoneCallsModel::PhoneCallsModel(QNetworkAccessManager *network, QObject *parent)
    : QAbstractListModel(parent),
    m_network(network)
{
}

PhoneCallsModel::~PhoneCallsModel()
{
    foreach (PhoneCallsItem *item, m_items)
        delete item;
}

QVariant PhoneCallsModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

void PhoneCallsModel::handleList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to retrieve phone calls: %s", qPrintable(reply->errorString()));
        return;
    }

    QDomElement callElement = doc.documentElement().firstChildElement("call");
    while (!callElement.isNull()) {
        const int id = callElement.attribute("id").toInt();
        if (id > 0) {
            qDebug("got call %i", id);
            PhoneCallsItem *item = new PhoneCallsItem;
            item->id = id;
            item->address = callElement.attribute("address");
            item->duration = callElement.attribute("duration").toInt();
            item->flags = callElement.attribute("flags").toInt();

            beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
            m_items.append(item);
            endInsertRows();
        }
        callElement = callElement.nextSiblingElement("call");
    }
}

int PhoneCallsModel::rowCount(const QModelIndex& parent) const
{
    return m_items.size();
}

void PhoneCallsModel::setUrl(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    QNetworkReply *reply = m_network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(handleList()));
}

