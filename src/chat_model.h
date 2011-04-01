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

#ifndef CHAT_MODEL_H
#define CHAT_MODEL_H

#include <QAbstractItemModel>

class ChatModelItem
{
public:
    ChatModelItem();
    ~ChatModelItem();
    int row() const;

    QList<ChatModelItem*> children;
    ChatModelItem *parent;

private:
    friend class ChatModel;
};

/** Base class for tree-like models to avoid some of the tedium of
 *  subclassing QAbstractItemModel.
 */
class ChatModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ChatModel(QObject *parent);
    ~ChatModel();

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent) const;

    QModelIndex reparentItem(const QModelIndex &index, const QModelIndex &newParent);

protected:
    void addItem(ChatModelItem *item, ChatModelItem *parentItem, int pos = -1);
    QModelIndex createIndex(ChatModelItem *item, int column = 0) const;
    void removeItem(ChatModelItem *item);

    ChatModelItem *rootItem;
};

#endif
