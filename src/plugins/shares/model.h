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

class ChatClient;

enum SharesColumns
{
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

enum SharesDataRoles {
    PacketId = QXmppShareItem::MaxRole,
    SizeRole,
    TransferDone,
    TransferPainted,
    TransferPath,
    TransferSpeed,
    TransferTotal,
    TransferError,
    UpdateTime,
};


class ShareModelQuery
{
public:
    enum Operation
    {
        None,
        Equals,
        NotEquals,
        // Contains,
    };

    ShareModelQuery();
    ShareModelQuery(int role, ShareModelQuery::Operation operation, QVariant data);

    bool match(QXmppShareItem *item) const;

    ShareModelQuery operator&&(const ShareModelQuery &other) const;
    ShareModelQuery operator||(const ShareModelQuery &other) const;

private:
    enum Combine
    {
        NoCombine,
        AndCombine,
        OrCombine,
    };

    int m_role;
    ShareModelQuery::Operation m_operation;
    QVariant m_data;

    QList<ShareModelQuery> m_children;
    ShareModelQuery::Combine m_combine;
};

/** Model representing a tree of share items (collections and files).
 */
class ShareModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_ENUMS(Recurse)
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)

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

    ShareModel(QObject *parent = 0);
    ~ShareModel();

    ChatClient *client() const;
    void setClient(ChatClient *client);

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();
    QXmppShareItem *addItem(const QXmppShareItem &item);
    QList<QXmppShareItem*> filter(const ShareModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0, int limit = 0);
    QXmppShareItem *get(const ShareModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0);
    void refreshItem(QXmppShareItem *item);
    void removeItem(QXmppShareItem *item);
    QModelIndex updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem);

signals:
    void clientChanged(ChatClient *client);

private:
    QXmppShareItem *rootItem;
    ChatClient *m_client;

    // cached icons, to avoid reloading them whenever an item is added
    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

#endif
