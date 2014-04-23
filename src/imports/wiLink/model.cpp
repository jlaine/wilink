/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

#include "model.h"

ChatModelItem::ChatModelItem()
    : parent(0)
{
}

ChatModelItem::~ChatModelItem()
{
    foreach (ChatModelItem *item, children)
        delete item;
}

int ChatModelItem::row() const
{
    if (!parent)
        return -1;
    return parent->children.indexOf((ChatModelItem*)this);
}

ChatModel::ChatModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_buffering(false)
    , m_changedCount(0)
{
    bool check;
    Q_UNUSED(check);

    // create root
    rootItem = new ChatModelItem;

    // connect signals
    check = connect(this, SIGNAL(modelReset()),
                    this, SIGNAL(countChanged()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)),
                    this, SIGNAL(countChanged()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                    this, SIGNAL(countChanged()));
    Q_ASSERT(check);
}

ChatModel::~ChatModel()
{
    delete rootItem;
}

void ChatModel::addItem(ChatModelItem *item, ChatModelItem *parentItem, int pos)
{
    Q_ASSERT(!item->parent);

    // emit any pending changes
    emitChanges();

    if (pos < 0 || pos > parentItem->children.size())
        pos = parentItem->children.size();
    beginInsertRows(createIndex(parentItem, 0), pos, pos);
    item->parent = parentItem;
    parentItem->children.insert(pos, item);
    endInsertRows();
}

void ChatModel::beginBuffering()
{
    m_buffering = true;
}

void ChatModel::emitChanges()
{
    if (!m_changedItems.isEmpty()) {
        //qDebug("combined %i changes into %i", m_changedCount, m_changedItems.size());
        foreach (ChatModelItem *item, m_changedItems) {
            const QModelIndex index = createIndex(item);
            emit dataChanged(index, index);
        }
        m_changedCount = 0;
        m_changedItems.clear();
    }
}

void ChatModel::endBuffering()
{
    // emit any pending changes
    emitChanges();

    m_buffering = false;
}

void ChatModel::changeItem(ChatModelItem *item)
{
    if (m_buffering) {
        m_changedCount++;
        m_changedItems << item;
    } else {
        const QModelIndex index = createIndex(item);
        emit dataChanged(index, index);
    }
}

int ChatModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex ChatModel::createIndex(ChatModelItem *item, int column) const
{
    if (item && item != rootItem)
        return QAbstractItemModel::createIndex(item->row(), column, item);
    else
        return QModelIndex();
}

QVariant ChatModel::get(int row) const
{
    const QModelIndex idx = index(row, 0);
    if (idx.isValid()) {
        QVariantMap result;
        const QHash<int, QByteArray> roleMap = roleNames();
        foreach (int role, roleMap.keys()) {
            const QString name = QString::fromAscii(roleMap.value(role));
            result.insert(name, idx.data(role));
        }
        result.insert("index", row);
        return result;
    }
    return QVariant();
}

QVariant ChatModel::getProperty(int row, const QString &name) const
{
    const QModelIndex idx = index(row, 0);
    if (idx.isValid()) {
        const int role = roleNames().key(name.toLatin1());
        return idx.data(role);
    }
    return QVariant();
}

QModelIndex ChatModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ChatModelItem *parentItem = parent.isValid() ? static_cast<ChatModelItem*>(parent.internalPointer()) : rootItem;
    return createIndex(parentItem->children[row], column);
}

QModelIndex ChatModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ChatModelItem *item = static_cast<ChatModelItem*>(index.internalPointer());
    return createIndex(item->parent);
}

bool ChatModel::removeRows(int row, int count, const QModelIndex &parent)
{
    ChatModelItem *parentItem = parent.isValid() ? static_cast<ChatModelItem*>(parent.internalPointer()) : rootItem;

    // emit any pending changes
    emitChanges();

    const int minIndex = qMax(0, row);
    const int maxIndex = qMin(row + count, parentItem->children.size()) - 1;
    beginRemoveRows(parent, minIndex, maxIndex);
    for (int i = maxIndex; i >= minIndex; --i) {
        ChatModelItem *item = parentItem->children.takeAt(i);
        delete item;
    }
    endRemoveRows();

    return true;
}

void ChatModel::removeItem(ChatModelItem *item)
{
    Q_ASSERT(item && item->parent);
    beginRemoveRows(createIndex(item->parent, 0), item->row(), item->row());
    item->parent->children.removeAll(item);
    delete item;
    endRemoveRows();
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    ChatModelItem *parentItem = parent.isValid() ? static_cast<ChatModelItem*>(parent.internalPointer()) : rootItem;
    return parentItem->children.size();
}

