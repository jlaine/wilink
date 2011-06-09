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
#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QMessageBox>
#include <QNetworkRequest>

#include "qnetio/wallet.h"

#include "chat.h"
#include "declarative.h"

QDeclarativeSortFilterProxyModel::QDeclarativeSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void QDeclarativeSortFilterProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (model != sourceModel()) {
        QSortFilterProxyModel::setSourceModel(model);
        emit sourceModelChanged(sourceModel());
    }
}

void QDeclarativeSortFilterProxyModel::sort(int column)
{
    QSortFilterProxyModel::sort(column);
}

ListHelper::ListHelper(QObject *parent)
    : QObject(parent),
    m_model(0)
{
}

int ListHelper::count() const
{
    if (m_model)
        return m_model->rowCount();
    return 0;
}

QVariant ListHelper::get(int row) const
{
    QVariantMap result;
    if (m_model) {
        QModelIndex index = m_model->index(row, 0);
        const QHash<int, QByteArray> roleNames = m_model->roleNames();
        foreach (int role, roleNames.keys())
            result.insert(QString::fromAscii(roleNames[role]), index.data(role));
    }
    return result;
}

QAbstractItemModel *ListHelper::model() const
{
    return m_model;
}

void ListHelper::setModel(QAbstractItemModel *model)
{
    if (model != m_model) {
        m_model = model;
        emit modelChanged(m_model);
    }
}

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    bool check;

    check = QObject::connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                             QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    Q_ASSERT(check);
    Q_UNUSED(check);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request(req);
    request.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

