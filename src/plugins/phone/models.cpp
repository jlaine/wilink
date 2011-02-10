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
#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>
#include <QHeaderView>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "QXmppUtils.h"
#include "qsound/QSoundPlayer.h"

#include "application.h"
#include "models.h"
#include "sip.h"

#define DATE_WIDTH 150
#define DURATION_WIDTH 100

#define FLAGS_DIRECTION 0x1
#define FLAGS_ERROR 0x2

enum CallsColumns {
    NameColumn = 0,
    DateColumn,
    DurationColumn,
    SortingColumn,
    MaxColumn,
};

static QString formatDuration(int secs)
{
    return QString::number(secs) + "s";
}

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
    int soundId;
};

PhoneCallsItem::PhoneCallsItem()
    : id(0),
    duration(0),
    flags(0),
    call(0),
    reply(0),
    soundId(0)
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
    date = datetimeFromString(callElement.firstChildElement("date").text());
    duration = callElement.firstChildElement("duration").text().toInt();
    flags = callElement.firstChildElement("flags").text().toInt();
}

/** Constructs a new model representing the call history.
 *
 * @param network
 * @param parent
 */
PhoneCallsModel::PhoneCallsModel(QNetworkAccessManager *network, QObject *parent)
    : QAbstractListModel(parent),
    m_network(network)
{
    m_soundPlayer = wApp->soundPlayer();

    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, SIGNAL(timeout()),
            this, SLOT(callTick()));
}

PhoneCallsModel::~PhoneCallsModel()
{
    foreach (PhoneCallsItem *item, m_items)
        delete item;
}

QList<SipCall*> PhoneCallsModel::activeCalls() const
{
    QList<SipCall*> calls;
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            calls << m_items[i]->call;
    return calls;
}

void PhoneCallsModel::addCall(SipCall *call)
{
    PhoneCallsItem *item = new PhoneCallsItem;
    item->address = call->recipient();
    item->flags = call->direction();
    item->call = call;
    connect(item->call, SIGNAL(stateChanged(QXmppCall::State)),
            this, SLOT(callStateChanged(QXmppCall::State)));
    if (item->call->direction() == QXmppCall::IncomingDirection)
        item->soundId = wApp->soundPlayer()->play(":/call-incoming.ogg", true);
    else
        connect(item->call, SIGNAL(ringing()), this, SLOT(callRinging()));
    QNetworkRequest request = buildRequest(m_url);
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    item->reply = m_network->post(request, item->data());
    connect(item->reply, SIGNAL(finished()),
            this, SLOT(handleCreate()));

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();

    // schedule periodic refresh
    if (!m_ticker->isActive()) {
        m_ticker->start();
        emit stateChanged(true);
    }
}

QNetworkRequest PhoneCallsModel::buildRequest(const QUrl &url) const
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    return req;
}

void PhoneCallsModel::callRinging()
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the call
    foreach (PhoneCallsItem *item, m_items) {
        if (item->call == call && !item->soundId) {
            item->soundId = wApp->soundPlayer()->play(":/call-outgoing.ogg", true);
            break;
        }
    }
}

void PhoneCallsModel::callStateChanged(QXmppCall::State state)
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the call
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->call == call) {
            row = i;
            break;
        }
    }
    if (row < 0)
        return;

    PhoneCallsItem *item = m_items[row];

    if (item->soundId && state != QXmppCall::OfferState) {
        wApp->soundPlayer()->stop(item->soundId);
        item->soundId = 0;
    }

    // update the item
    if (state == QXmppCall::FinishedState) {
        item->call = 0;
        item->duration = call->duration();
        if (!call->error().isEmpty()) {
            item->flags |= FLAGS_ERROR;
            emit error(call->error());
        }
        QUrl url = m_url;
        url.setPath(url.path() + QString::number(item->id) + "/");
        QNetworkRequest request = buildRequest(url);
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        m_network->put(request, item->data());

        call->deleteLater();
    }
    emit dataChanged(createIndex(row, NameColumn),
                     createIndex(row, SortingColumn));
}

void PhoneCallsModel::callTick()
{
    bool active = false;
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items[row]->call) {
            active = true;
            emit dataChanged(createIndex(row, DurationColumn),
                             createIndex(row, DurationColumn));
        }
    }
    if (!active) {
        m_ticker->stop();
        emit stateChanged(false);
    }
}

/** Returns the number of columns under the given \a parent.
 *
 * @param parent
 */
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
    } else if (role == ActiveRole) {
        return item->call != 0;
    }

    if (index.column() == NameColumn) {
        if (role == Qt::DisplayRole)
            return sipAddressToName(item->address);
        else if (role == Qt::DecorationRole) {
            if ((item->flags & FLAGS_DIRECTION) == QXmppCall::OutgoingDirection)
                return QPixmap(":/call-outgoing.png");
            else
                return QPixmap(":/call-incoming.png");
        } else if(role == Qt::BackgroundRole && item->call) {
            QLinearGradient grad(QPointF(0, 0), QPointF(0.8, 0));
            grad.setColorAt(0, QColor(255, 0, 0, 144));
            grad.setColorAt(1, Qt::transparent);
            grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            return QBrush(grad);
        }
    } else if (index.column() == DateColumn) {
        if (role == Qt::DisplayRole)
            return item->date.toLocalTime().toString(Qt::SystemLocaleShortDate);
    } else if (index.column() == DurationColumn && role == Qt::DisplayRole) {

        if (item->call) {
            switch (item->call->state())
            {
            case QXmppCall::OfferState:
            case QXmppCall::ConnectingState:
                return tr("Connecting..");
            case QXmppCall::ActiveState:
                return formatDuration(item->call->duration());
            case QXmppCall::DisconnectingState:
                return tr("Disconnecting..");
            default:
                break;
            }
        }
        if (item->flags & FLAGS_ERROR)
            return tr("Failed");
        return QString::number(item->duration) + "s";

    } else if (index.column() == SortingColumn) {
        if (role == Qt::DisplayRole)
            return QString::number(item->date.toTime_t());
    }

    return QVariant();
}

void PhoneCallsModel::handleCreate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    // find the item
    PhoneCallsItem *item = 0;
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->reply == reply) {
            item = m_items[i];
            item->reply = 0;
            row = i;
            break;
        }
    }
    if (!item)
        return;

    // update the item
    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to create phone call: %s", qPrintable(reply->errorString()));
        return;
    }
    item->parse(doc.documentElement());
    emit dataChanged(createIndex(row, NameColumn),
                     createIndex(row, SortingColumn));
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
        int id = callElement.firstChildElement("id").text().toInt();
        foreach (PhoneCallsItem *item, m_items) {
            if (item->id == id) {
                id = 0;
                break;
            }
        }
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

/** Hangs up all active calls.
 */
void PhoneCallsModel::hangup()
{
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            QMetaObject::invokeMethod(m_items[i]->call, "hangup");
}

/** Removes \a count rows starting at the given \a row under the given \a parent.
 *
 * @param row
 * @param count
 * @param parent
 */
bool PhoneCallsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    int last = row + count - 1;
    if (parent.isValid() || row < 0 || count < 0 || last >= m_items.size())
        return false;

    beginRemoveRows(parent, row, last);
    for (int i = last; i >= row; --i) {
        PhoneCallsItem *item = m_items[i];
        QUrl url = m_url;
        url.setPath(url.path() + QString::number(item->id) + "/");
        m_network->deleteResource(buildRequest(url));
        delete item;
        m_items.removeAt(i);
    }
    endRemoveRows();
    return true;
}

/** Returns the number of rows under the given \a parent.
 *
 * @param parent
 */
int PhoneCallsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

/** Sets the base URL for the call history.
 *
 * @param url
 */
void PhoneCallsModel::setUrl(const QUrl &url)
{
    m_url = url;

    QNetworkReply *reply = m_network->get(buildRequest(m_url));
    connect(reply, SIGNAL(finished()), this, SLOT(handleList()));
}

PhoneCallsView::PhoneCallsView(PhoneCallsModel *model, QWidget *parent)
    : QTableView(parent),
    m_callsModel(model)
{
    m_sortedModel = new QSortFilterProxyModel(this);
    m_sortedModel->setSourceModel(model);
    m_sortedModel->setDynamicSortFilter(true);
    m_sortedModel->setFilterKeyColumn(SortingColumn);
    setModel(m_sortedModel);

    setAlternatingRowColors(true);
    setColumnHidden(SortingColumn, true);
    setColumnWidth(DateColumn, DATE_WIDTH);
    setColumnWidth(DurationColumn, DURATION_WIDTH);
    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(SortingColumn, Qt::DescendingOrder);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
}

void PhoneCallsView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        QAction *action;
        bool check;
        QMenu *menu = new QMenu(this);

        action = menu->addAction(QIcon(":/call.png"), tr("Call"));
        if (!m_callsModel->activeCalls().isEmpty())
            action->setEnabled(false);
        check = connect(action, SIGNAL(triggered()),
                        this, SLOT(callSelected()));
        Q_ASSERT(check);

        action = menu->addAction(QIcon(":/remove.png"), tr("Remove"));
        if (index.data(PhoneCallsModel::ActiveRole).toBool())
            action->setEnabled(false);
        check = connect(action, SIGNAL(triggered()),
                        this, SLOT(removeSelected()));
        Q_ASSERT(check);

        menu->popup(event->globalPos());
    }
}

void PhoneCallsView::callSelected()
{
    QModelIndexList indexes = selectedIndexes();
    if (!indexes.isEmpty())
        emit doubleClicked(indexes.first());
}

void PhoneCallsView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);
    if (current.isValid())
        emit clicked(current);
}

void PhoneCallsView::keyPressEvent(QKeyEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (index.isValid()) {
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        {
            if (!index.data(PhoneCallsModel::ActiveRole).toBool())
                m_sortedModel->removeRow(index.row());
        }
        else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
            emit doubleClicked(index);
    }
    QTableView::keyPressEvent(event);
}

void PhoneCallsView::removeSelected()
{
    QModelIndexList indexes = selectedIndexes();
    if (!indexes.isEmpty()) {
        QModelIndex index = indexes.first();
        if (!index.data(PhoneCallsModel::ActiveRole).toBool())
            m_sortedModel->removeRow(indexes.first().row());
    }
}

void PhoneCallsView::resizeEvent(QResizeEvent *e)
{
    QTableView::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - DATE_WIDTH - DURATION_WIDTH);
}

