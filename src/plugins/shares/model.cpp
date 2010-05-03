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

#include <QDesktopServices>
#include <QFileIconProvider>
#include <QStringList>
#include <QUrl>

#include "qxmpp/QXmppShareIq.h"

#include "plugins/shares/database.h"
#include "plugins/shares/model.h"
#include "plugins/transfers.h"

// common queries
#define Q ChatSharesModelQuery
#define Q_FIND_LOCATIONS(locations)  Q(QXmppShareItem::LocationsRole, Q::Equals, QVariant::fromValue(locations))

/** Update collection timestamps.
 */
static void updateTime(QXmppShareItem *oldItem, const QDateTime &stamp)
{
    if (oldItem->type() == QXmppShareItem::CollectionItem && oldItem->size() > 0)
    {
        oldItem->setData(UpdateTime, QDateTime::currentDateTime());
        for (int i = 0; i < oldItem->size(); i++)
            updateTime(oldItem->child(i), stamp);
    }
}

ChatSharesModel::ChatSharesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new QXmppShareItem(QXmppShareItem::CollectionItem);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);
    peerIcon = iconProvider.icon(QFileIconProvider::Network);
}

ChatSharesModel::~ChatSharesModel()
{
    delete rootItem;
}

QXmppShareItem *ChatSharesModel::addItem(const QXmppShareItem &item)
{
   beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
   QXmppShareItem *child = rootItem->appendChild(item);
   endInsertRows();
   return child;
}

int ChatSharesModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatSharesModel::data(const QModelIndex &index, int role) const
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == NameColumn)
        return item->name();
    else if (role == Qt::DisplayRole && index.column() == SizeColumn && item->fileSize())
        return ChatTransfers::sizeToString(item->fileSize());
    else if (role == Qt::DisplayRole && index.column() == ProgressColumn &&
             item->type() == QXmppShareItem::FileItem)
    {
        const QString localPath = item->data(TransferPath).toString();
        int done = item->data(TransferDone).toInt();
        if (!localPath.isEmpty())
            return tr("Downloaded");
        else if (item->data(TransferError).toInt())
            return tr("Failed");
        else if (done > 0)
        {
            int total = index.data(TransferTotal).toInt();
            int progress = total ? (qreal(done) * 100.0 / total) : 0;
            return QString::number(progress) + "%";
        }
        else if (!item->data(PacketId).toString().isEmpty())
            return tr("Requested");
        else
            return tr("Queued");
    }
    else if (role == Qt::ToolTipRole && index.column() == ProgressColumn &&
             item->type() == QXmppShareItem::FileItem)
    {
        const QString localPath = item->data(TransferPath).toString();
        QTime t = index.data(TransferStart).toTime();
        int done = item->data(TransferDone).toInt();
        if (!item->data(TransferError).toInt() &&
            localPath.isEmpty() &&
            done > 0 && t.isValid() && t.elapsed())
        {
            int speed = (done * 1000.0) / t.elapsed();
            return tr("Downloading at %1").arg(ChatTransfers::sizeToString(speed) + "/s");
        }
    }
    else if (role == Qt::DecorationRole && index.column() == NameColumn)
    {
        if (item->type() == QXmppShareItem::CollectionItem)
        {
            if (item->locations().size() == 1 && item->locations().first().node().isEmpty())
                return peerIcon;
            else
                return collectionIcon;
        }
        else
            return fileIcon;
    }
    return item->data(role);
}

QList<QXmppShareItem *> ChatSharesModel::filter(const ChatSharesModelQuery &query, const ChatSharesModel::QueryOptions &options, QXmppShareItem *parent, int limit)
{
    if (!parent)
        parent = rootItem;

    QList<QXmppShareItem*> matches;
    QXmppShareItem *child;

    // pre-recurse
    if (options.recurse == PreRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }

    // look at immediate children
    for (int i = 0; i < parent->size(); i++)
    {
        child = parent->child(i);
        if (query.match(child))
            matches.append(child);
        if (limit && matches.size() == limit)
            return matches;
    }

    // post-recurse
    if (options.recurse == PostRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }
    return matches;
}

QXmppShareItem *ChatSharesModel::get(const ChatSharesModelQuery &query, const ChatSharesModel::QueryOptions &options, QXmppShareItem *parent)
{
    QList<QXmppShareItem*> match = filter(query, options, parent, 1);
    if (match.size())
        return match.first();
    return 0;
}

/** Return the title for the given column.
 */
QVariant ChatSharesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == NameColumn)
            return tr("Name");
        else if (section == ProgressColumn)
            return tr("Progress");
        else if (section == SizeColumn)
            return tr("Size");
    }
    return QVariant();
}

QModelIndex ChatSharesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());

    QXmppShareItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ChatSharesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareItem *childItem = static_cast<QXmppShareItem*>(index.internalPointer());
    QXmppShareItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

void ChatSharesModel::refreshItem(QXmppShareItem *item)
{
    emit dataChanged(createIndex(item->row(), ProgressColumn, item), createIndex(item->row(), ProgressColumn, item));
}

void ChatSharesModel::removeItem(QXmppShareItem *item)
{
    if (!item || item == rootItem)
        return;

    QXmppShareItem *parentItem = item->parent();
    if (parentItem == rootItem)
    {
        beginRemoveRows(QModelIndex(), item->row(), item->row());
        rootItem->removeChild(item);
        endRemoveRows();
    } else {
        beginRemoveRows(createIndex(parentItem->row(), 0, parentItem), item->row(), item->row());
        parentItem->removeChild(item);
        endRemoveRows();
    }
}

int ChatSharesModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());
    return parentItem->size();
}

QModelIndex ChatSharesModel::updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem)
{
    QDateTime stamp = QDateTime::currentDateTime();

    QModelIndex oldIndex;
    if (!oldItem)
        oldItem = rootItem;
    if (oldItem != rootItem)
        oldIndex = createIndex(oldItem->row(), 0, oldItem);

    oldItem->setFileHash(newItem->fileHash());
    oldItem->setFileSize(newItem->fileSize());
    oldItem->setLocations(newItem->locations());
    //oldItem->setName(newItem->name());
    oldItem->setType(newItem->type());
    if (oldItem->type() == QXmppShareItem::CollectionItem && oldItem->size() > 0)
        oldItem->setData(UpdateTime, stamp);

    QList<QXmppShareItem*> removed = oldItem->children();
    QList<QXmppShareItem*> added;
    foreach (QXmppShareItem *newChild, newItem->children())
    {
        QXmppShareItem *oldChild = get(Q_FIND_LOCATIONS(newChild->locations()), QueryOptions(DontRecurse), oldItem);
        if (oldChild)
        {
            updateItem(oldChild, newChild);
            removed.removeAll(oldChild);
        } else {
            added << newChild;
        }
    }

    // remove old children if we received a file or a non-empty collection
    if (newItem->type() == QXmppShareItem::FileItem || newItem->size())
    {
        foreach (QXmppShareItem *oldChild, removed)
        {
            beginRemoveRows(oldIndex, oldChild->row(), oldChild->row());
            oldItem->removeChild(oldChild);
            endRemoveRows();
        }
    }

    // add new children
    if (added.size())
    {
        beginInsertRows(oldIndex, oldItem->size(), oldItem->size() + added.size());
        foreach (QXmppShareItem *newChild, added)
        {
            QXmppShareItem *item = oldItem->appendChild(*newChild);
            updateTime(item, stamp);
        }
        endInsertRows();
    }

    return oldIndex;
}

ChatSharesModelQuery::ChatSharesModelQuery()
    :  m_role(0), m_operation(None), m_combine(NoCombine)
{
}

ChatSharesModelQuery::ChatSharesModelQuery(int role, ChatSharesModelQuery::Operation operation, QVariant data)
    :  m_role(role), m_operation(operation), m_data(data), m_combine(NoCombine)
{
}

bool ChatSharesModelQuery::match(QXmppShareItem *item) const
{
    if (m_operation == Equals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return item->locations() == m_data.value<QXmppShareLocationList>();
        return item->data(m_role) == m_data;
    }
    else if (m_operation == NotEquals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return !(item->locations() == m_data.value<QXmppShareLocationList>());
        return (item->data(m_role) != m_data);
    }
    else if (m_combine == AndCombine)
    {
        foreach (const ChatSharesModelQuery &child, m_children)
            if (!child.match(item))
                return false;
        return true;
    }
    else if (m_combine == OrCombine)
    {
        foreach (const ChatSharesModelQuery &child, m_children)
            if (child.match(item))
                return true;
        return false;
    }
    // empty query matches all
    return true;
}

ChatSharesModelQuery ChatSharesModelQuery::operator&&(const ChatSharesModelQuery &other) const
{
    ChatSharesModelQuery result;
    result.m_combine = AndCombine;
    result.m_children << *this << other;
    return result;
}

ChatSharesModelQuery ChatSharesModelQuery::operator||(const ChatSharesModelQuery &other) const
{
    ChatSharesModelQuery result;
    result.m_combine = OrCombine;
    result.m_children << *this << other;
    return result;
}

ChatSharesModel::QueryOptions::QueryOptions(Recurse recurse_)
    : recurse(recurse_)
{
}


