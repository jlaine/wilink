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

#include "chat_model.h"

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
{
}

ChatModel::~ChatModel()
{
    delete rootItem;
}

void ChatModel::addItem(ChatModelItem *item, ChatModelItem *parentItem, int pos)
{
    Q_ASSERT(!item->parent);
    if (pos < 0 || pos > parentItem->children.size())
        pos = parentItem->children.size();
    beginInsertRows(createIndex(parentItem, 0), pos, pos);
    item->parent = parentItem;
    parentItem->children.insert(pos, item);
    endInsertRows();
}

QModelIndex ChatModel::createIndex(ChatModelItem *item, int column) const
{
    if (item && item != rootItem)
        return QAbstractItemModel::createIndex(item->row(), column, item);
    else
        return QModelIndex();
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

    const int minIndex = qBound(0, row, parentItem->children.size() - 1);
    const int maxIndex = qBound(0, row + count, parentItem->children.size() - 1);
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

/** Move an item to the given parent.
 *
 * @param index
 * @param newParent
 */
QModelIndex ChatModel::reparentItem(const QModelIndex &index, const QModelIndex &newParent)
{
    if (!index.isValid())
        return index;
    ChatModelItem *item = static_cast<ChatModelItem*>(index.internalPointer());

    /* determine requested parent item */
    ChatModelItem *newParentItem;
    if (!newParent.isValid())
        newParentItem = rootItem;
    else
        newParentItem = static_cast<ChatModelItem*>(newParent.internalPointer());

    /* check if we need to do anything */
    ChatModelItem *currentParentItem = item->parent;
    if (currentParentItem == newParentItem)
        return index;

    /* determine current parent index */
    QModelIndex currentParent;
    if (currentParentItem != rootItem)
        currentParent = createIndex(currentParentItem, 0);

    /* move item */
    beginMoveRows(currentParent, item->row(), item->row(),
                  newParent, newParentItem->children.size());
    if (currentParentItem)
        currentParentItem->children.removeAll(item);
    if (newParentItem)
        newParentItem->children.append(item);
    item->parent = newParentItem;
    endMoveRows();

    return createIndex(item, 0);
}

/** Returns the row for the given index.
 *
 * This method is provided for convenience when using models
 * in a QML view.
 *
 * @param index
 */
int ChatModel::row(const QModelIndex &index) const
{
    return index.row();
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    ChatModelItem *parentItem = parent.isValid() ? static_cast<ChatModelItem*>(parent.internalPointer()) : rootItem;
    return parentItem->children.size();
}

