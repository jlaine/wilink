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

    const int minIndex = qMax(0, row);
    const int maxIndex = qMin(row + count, parentItem->children.size()) - 1;
    beginRemoveRows(parent, minIndex, maxIndex);
    for (int i = maxIndex; i >= minIndex; --i)
        parentItem->children.removeAt(i);
    endRemoveRows();

    return true;
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    ChatModelItem *parentItem = parent.isValid() ? static_cast<ChatModelItem*>(parent.internalPointer()) : rootItem;
    return parentItem->children.size();
}

