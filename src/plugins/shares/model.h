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

#ifndef __WILINK_SHARES_MODEL_H__
#define __WILINK_SHARES_MODEL_H__

#include <QAbstractItemModel>
#include <QIcon>

#include "QXmppShareIq.h"

enum SharesColumns
{
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

enum SharesDataRoles {
    PacketId = QXmppShareItem::MaxRole,
    TransferDone,
    TransferPainted,
    TransferPath,
    TransferSpeed,
    TransferTotal,
    TransferError,
    UpdateTime,
};


class SharesModelQuery
{
public:
    enum Operation
    {
        None,
        Equals,
        NotEquals,
        // Contains,
    };

    SharesModelQuery();
    SharesModelQuery(int role, SharesModelQuery::Operation operation, QVariant data);

    bool match(QXmppShareItem *item) const;

    SharesModelQuery operator&&(const SharesModelQuery &other) const;
    SharesModelQuery operator||(const SharesModelQuery &other) const;

private:
    enum Combine
    {
        NoCombine,
        AndCombine,
        OrCombine,
    };

    int m_role;
    SharesModelQuery::Operation m_operation;
    QVariant m_data;

    QList<SharesModelQuery> m_children;
    SharesModelQuery::Combine m_combine;
};

/** Model representing a tree of share items (collections and files).
 */
class SharesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Recurse
    {
        DontRecurse,
        PreRecurse,
        PostRecurse,
    };

    class QueryOptions
    {
    public:
        QueryOptions(Recurse recurse = PreRecurse);
        Recurse recurse;
    };

    SharesModel(QObject *parent = 0);
    ~SharesModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();
    QXmppShareItem *addItem(const QXmppShareItem &item);
    QList<QXmppShareItem*> filter(const SharesModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0, int limit = 0);
    QXmppShareItem *get(const SharesModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0);
    void refreshItem(QXmppShareItem *item);
    void removeItem(QXmppShareItem *item);
    QModelIndex updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem);

private:
    QXmppShareItem *rootItem;

    // cached icons, to avoid reloading them whenever an item is added
    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

#endif
