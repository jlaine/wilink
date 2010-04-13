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

#ifndef __WDESKTOP_CHAT_ROSTER_ITEM_H__
#define __WDESKTOP_CHAT_ROSTER_ITEM_H__

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

class ChatRosterItem
{
public:
    enum Type {
        Root,
        Contact,
        Room,
        RoomMember,
        Other,
    };

    ChatRosterItem(enum Type type, const QString &id = QString());
    ~ChatRosterItem();

    void append(ChatRosterItem *item);
    ChatRosterItem *child(int row);
    void clear();
    QVariant data(int role) const;
    ChatRosterItem* find(const QString &id);
    QString id() const;
    ChatRosterItem* parent();
    void remove(ChatRosterItem *item);
    int row() const;
    void setData(int role, const QVariant &value);
    int size() const;
    enum Type type() const;

private:
    QString itemId;
    QMap<int, QVariant> itemData;
    enum Type itemType;
    QList <ChatRosterItem*> childItems;
    ChatRosterItem *parentItem;
};

#endif
