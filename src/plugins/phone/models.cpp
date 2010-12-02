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
#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QResizeEvent>

#include "models.h"
#include "sip.h"

#define DATE_WIDTH 150
#define DURATION_WIDTH 60

enum CallsColumns {
    NameColumn = 0,
    DateColumn,
    DurationColumn,
    MaxColumn,
};

class PhoneCallsItem
{
public:
    PhoneCallsItem();
    QByteArray data() const;
    void parse(const QDomElement &callElement);

    int id;
    QString address;
    QDateTime date;
    int duration;
    int flags;

    SipCall *call;
    QNetworkReply *reply;
};

PhoneCallsItem::PhoneCallsItem()
    : id(0),
    duration(0),
    flags(0),
    call(0),
    reply(0)
{
}

QByteArray PhoneCallsItem::data() const
{
    QUrl data;
    data.addQueryItem("address", address);
    data.addQueryItem("duration", QString::number(duration));
    data.addQueryItem("flags", QString::number(flags));
    return data.encodedQuery();
}

void PhoneCallsItem::parse(const QDomElement &callElement)
{
    id = callElement.firstChildElement("id").text().toInt();
    address = callElement.firstChildElement("address").text();
    date = QDateTime::fromString(callElement.firstChildElement("date").text(), Qt::ISODate);
    duration = callElement.firstChildElement("duration").text().toInt();
    flags = callElement.firstChildElement("flags").text().toInt();
}

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

void PhoneCallsModel::addCall(SipCall *call)
{
    PhoneCallsItem *item = new PhoneCallsItem;
    item->address = call->recipient();
    item->flags = call->direction();
    item->call = call;
    item->reply = m_network->post(buildRequest(m_url), item->data());
    connect(item->reply, SIGNAL(finished()), this, SLOT(handleCreate()));
    m_pending << item;
}

QNetworkRequest PhoneCallsModel::buildRequest(const QUrl &url) const
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    return req;
}

int PhoneCallsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MaxColumn;
}

QVariant PhoneCallsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row > m_items.size())
        return QVariant();
    PhoneCallsItem *item = m_items[row];

    if (role == Qt::ToolTipRole) {
        return item->address;
    } else if (role == AddressRole) {
        return item->address;
    }

    if (index.column() == NameColumn) {
        if (role == Qt::DisplayRole)
            return sipAddressToName(item->address);
        else if (role == Qt::DecorationRole) {
            if ((item->flags & 0x1) == QXmppCall::OutgoingDirection)
                return QPixmap(":/upload.png");
            else
                return QPixmap(":/download.png");
        }
    } else if (index.column() == DateColumn) {
        if (role == Qt::DisplayRole)
            return item->date.toString(Qt::SystemLocaleShortDate);
    } else if (index.column() == DurationColumn) {
        if (role == Qt::DisplayRole)
            return QString::number(item->duration) + "s";
    }

    return QVariant();
}

void PhoneCallsModel::handleCreate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    // find the pending item
    PhoneCallsItem *item = 0;
    for (int i = 0; i < m_pending.size(); ++i) {
        if (m_pending[i]->reply == reply) {
            item = m_pending[i];
            item->reply = 0;
            m_pending.removeAt(i);
            break;
        }
    }
    if (!item)
        return;

    // parse reply
    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to create phone call: %s", qPrintable(reply->errorString()));
        return;
    }
    item->parse(doc.documentElement());
    connect(item->call, SIGNAL(finished()), this, SLOT(handleFinished()));

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
}

void PhoneCallsModel::handleFinished()
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the item
    PhoneCallsItem *item = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->call == call) {
            item = m_items[i];
            item->call = 0;
            break;
        }
    }
    if (!item)
        return;

    // update the item
    item->duration = call->duration();
    QUrl url = m_url;
    url.setPath(url.path() + QString::number(item->id) + "/");
    item->reply = m_network->post(buildRequest(url), item->data());
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
        const int id = callElement.firstChildElement("id").text().toInt();
        if (id > 0) {
            PhoneCallsItem *item = new PhoneCallsItem;
            item->parse(callElement);

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
    m_url = url;

    QNetworkReply *reply = m_network->get(buildRequest(m_url));
    connect(reply, SIGNAL(finished()), this, SLOT(handleList()));
}

PhoneCallsView::PhoneCallsView(PhoneCallsModel *model, QWidget *parent)
    : QTableView(parent)
{
    setModel(model);

    setAlternatingRowColors(true);
    setColumnWidth(DateColumn, DATE_WIDTH);
    setColumnWidth(DurationColumn, DURATION_WIDTH);
    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
}

void PhoneCallsView::resizeEvent(QResizeEvent *e)
{
    QTableView::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - DATE_WIDTH - DURATION_WIDTH);
}

