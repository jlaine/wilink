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

#ifndef __WILINK_CHAT_ROSTER_ITEM_H__
#define __WILINK_CHAT_ROSTER_ITEM_H__

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

#include "chat_model.h"

class ChatRosterItem : public ChatModelItem
{
public:
    enum Type {
        Root,
        Contact,
        Room,
        RoomMember,
        Other,
    };

    ChatRosterItem(enum Type type);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QString id() const;
    void setId(const QString &id);

    enum Type type() const;

    void append(ChatRosterItem *item);
    void clear();
    ChatRosterItem* find(const QString &id);
    void remove(ChatModelItem *item);
    void removeAt(int row);
    int size() const;

private:
    QString itemId;
    QMap<int, QVariant> itemData;
    enum Type itemType;
};

#endif
