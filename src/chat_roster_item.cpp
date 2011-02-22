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

#include <QStringList>

#include "chat_roster_item.h"

ChatRosterItem::ChatRosterItem(enum ChatRosterModel::Type type)
    : itemType(type)
{
}

void ChatRosterItem::append(ChatRosterItem *item)
{
    if (item->parent)
    {
        qWarning("item already has a parent");
        return;
    }
    item->parent = this;
    children.append(item);
}

QVariant ChatRosterItem::data(int role) const
{
    return itemData.value(role);
}

ChatRosterItem *ChatRosterItem::find(const QString &id)
{
    /* look at immediate children */
    foreach (ChatModelItem *it, children) {
        ChatRosterItem *item = static_cast<ChatRosterItem*>(it);
        if (item->itemId == id)
            return item;
    }

    /* recurse */
    foreach (ChatModelItem *item, children) {
        ChatRosterItem *found = static_cast<ChatRosterItem*>(item)->find(id);
        if (found)
            return found;
    }
    return 0;
}

QString ChatRosterItem::id() const
{
    return itemId;
}

void ChatRosterItem::setId(const QString &id)
{
    itemId = id;

    QString name;
    if (itemType == ChatRosterModel::RoomMember)
        name = id.split('/').last();
    else
        name = id.split('@').first();
    setData(Qt::DisplayRole, name);
}

void ChatRosterItem::remove(ChatModelItem *child)
{
    if (children.contains(child))
    {
        children.removeAll(child);
        delete child;
    }
}

void ChatRosterItem::setData(int role, const QVariant &value)
{
    itemData.insert(role, value);
}

enum ChatRosterModel::Type ChatRosterItem::type() const
{
    return itemType;
}
