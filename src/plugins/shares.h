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

#include "QXmppShareIq.h"

#include "model.h"

class ChatClient;
class QXmppPresence;
class QXmppShareManager;
class ShareModelPrivate;
class ShareQueueModel;
class ShareQueueModelPrivate;

/** Model representing a tree of share items (collections and files).
 */
class ShareModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_ENUMS(Recurse)
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(ShareQueueModel *queue READ queue CONSTANT)
    Q_PROPERTY(QString rootJid READ rootJid WRITE setRootJid NOTIFY rootJidChanged)
    Q_PROPERTY(QString rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)
    Q_PROPERTY(QString shareServer READ shareServer NOTIFY shareServerChanged)

public:
    enum Role {
        CanDownloadRole = ChatModel::UserRole,
        IsDirRole,
        JidRole,
        NameRole,
        NodeRole,
        PopularityRole,
        SizeRole,
    };

    ShareModel(QObject *parent = 0);
    ~ShareModel();

    ChatClient *client() const;
    void setClient(ChatClient *client);

    QString filter() const;
    void setFilter(const QString &filter);

    ShareQueueModel *queue() const;

    QString rootJid() const;
    void setRootJid(const QString &rootJid);

    QString rootNode() const;
    void setRootNode(const QString &rootNode);

    QString shareServer() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void clientChanged(ChatClient *client);
    void filterChanged(const QString &filter);
    void rootJidChanged(const QString &rootJid);
    void rootNodeChanged(const QString &rootNode);
    void shareServerChanged(const QString &shareServer);

public slots:
    void download(int row);
    void refresh();

private slots:
    void _q_disconnected();
    void _q_presenceReceived(const QXmppPresence &presence);
    void _q_serverChanged(const QString &server);
    void _q_searchReceived(const QXmppShareSearchIq &shareIq);

private:
    void clear();
    QModelIndex createIndex(QXmppShareItem *item, int column = 0) const;

    QXmppShareItem *rootItem;
    ShareModelPrivate *d;
    friend class ShareModelPrivate;
};

class ShareQueueModel : public ChatModel
{
    Q_OBJECT

public:
    enum Role {
        IsDirRole = ChatModel::UserRole,
        NodeRole,
        SpeedRole,
        DoneBytesRole,
        DoneFilesRole,
        TotalBytesRole,
        TotalFilesRole,
    };

    ShareQueueModel(QObject *parent = 0);
    ~ShareQueueModel();

    void add(const QXmppShareItem &item, const QString &filter);
    bool contains(const QXmppShareItem &item) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void setManager(QXmppShareManager *manager);

public slots:
    void cancel(int row);

private slots:
    void _q_refresh();
    void _q_searchReceived(const QXmppShareSearchIq &shareIq);
    void _q_transferFinished();

private:
    ShareQueueModelPrivate *d;
    friend class ShareQueueModelPrivate;
};

#endif
