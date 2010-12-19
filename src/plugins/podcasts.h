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

#ifndef __WILINK_PODCASTS_H___
#define __WILINK_PODCASTS_H___

#include <QAbstractItemModel>
#include <QUrl>

#include "chat_panel.h"

class Chat;
class QNetworkAccessManager;
class QTreeView;

class PodcastsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PodcastsModel(QObject *parent);
    ~PodcastsModel();

    void addChannel(const QUrl &url);
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex & index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

private slots:
    void imageReceived();
    void xmlReceived();

private:
    class Item {
    public:
        Item();
        ~Item();

        int row() const;

        QUrl audioUrl;
        QUrl imageUrl;
        QString title;
        QUrl url;

        Item *parent;
        QList<Item*> children;
    };

    QModelIndex createIndex(Item *item, int column) const
    {
        if (item && item != m_rootItem)
            return QAbstractItemModel::createIndex(item->row(), column, item);
        else
            return QModelIndex();
    }

    QNetworkAccessManager *m_network;
    Item *m_rootItem;
};

/** The PodcastsPanel class represents a panel for displaying podcasts.
 */
class PodcastsPanel : public ChatPanel
{
    Q_OBJECT

public:
    PodcastsPanel(Chat *chatWindow);

private slots:

private:
    Chat *m_chat;
    PodcastsModel *m_model;
    QTreeView *m_view;
};

#endif
