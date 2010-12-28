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

#ifndef __WILINK_PLAYER_H__
#define __WILINK_PLAYER_H__

#include <QAbstractListModel>
#include <QUrl>
#include <QTreeView>

#include "chat_panel.h"

class Chat;

class PlayerModel : public QAbstractListModel
{
    Q_OBJECT

public:
    PlayerModel(QObject *parent = 0);
    ~PlayerModel();

    void addFile(const QUrl &url);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    class Item {
    public:
        Item() : parent(0)
        {
        }
        ~Item()
        {
            foreach (Item *item, children)
                delete item;
        }
        int row() const
        {
            if (!parent)
                return -1;
            return parent->children.indexOf((Item*)this);
        }

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

    Item *m_rootItem;
};

/** The PlayerPanel class represents a panel for playing media.
 */
class PlayerPanel : public ChatPanel
{
    Q_OBJECT

public:
    PlayerPanel(Chat *chatWindow);

private slots:
    void doubleClicked(const QModelIndex &index);

private:
    Chat *m_chat;
    PlayerModel *m_model;
    QUrl m_playUrl;
    int m_playId;
    QTreeView *m_view;
};

class PlayerView : public QTreeView
{
    Q_OBJECT

public:
    PlayerView(QWidget *parent = 0);
};

#endif
