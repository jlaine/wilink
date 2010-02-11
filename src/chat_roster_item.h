/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_ROSTER_ITEM_H__
#define __WDESKTOP_CHAT_ROSTER_ITEM_H__

#include <QList>
#include <QString>

class ChatRoomsItem
{
public:
    ChatRoomsItem(const QString &id);
    ~ChatRoomsItem();

    void append(ChatRoomsItem *item);
    ChatRoomsItem *child(int row);
    bool contains(const QString &id) const;
    ChatRoomsItem* find(const QString &id);
    QString id() const;
    ChatRoomsItem* parent();
    void remove(ChatRoomsItem *item);
    int row() const;
    int size() const;

private:
    QString itemId;
    QList <ChatRoomsItem*> childItems;
    ChatRoomsItem *parentItem;
};

#endif
