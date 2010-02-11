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

#include "chat_roster_item.h"

ChatRosterItem::ChatRosterItem(enum ChatRosterItem::Type type, const QString &id)
    : itemId(id), itemType(type), parentItem(0)
{
}

ChatRosterItem::~ChatRosterItem()
{
    foreach (ChatRosterItem *item, childItems)
        delete item;
}

void ChatRosterItem::append(ChatRosterItem *item)
{
    if (item->parentItem)
    {
        qWarning("item already has a parent");
        return;
    }
    item->parentItem = this;
    childItems.append(item);
}

ChatRosterItem *ChatRosterItem::child(int row)
{
    if (row >= 0 && row < childItems.size())
        return childItems.at(row);
    else
        return 0;
}

void ChatRosterItem::clear()
{
    foreach (ChatRosterItem *item, childItems)
        delete item;
    childItems.clear();
}

bool ChatRosterItem::contains(const QString &id) const
{
    foreach (const ChatRosterItem *item, childItems)
    {
        if (item->id() == id)
            return true;
    }
    return false;
}

QVariant ChatRosterItem::data(int role) const
{
    return itemData.value(role);
}

ChatRosterItem *ChatRosterItem::find(const QString &id)
{
    foreach (ChatRosterItem *item, childItems)
        if (item->itemId == id)
            return item;
    return 0;
}

QString ChatRosterItem::id() const
{
    return itemId;
}

ChatRosterItem* ChatRosterItem::parent()
{
    return parentItem;
}

void ChatRosterItem::remove(ChatRosterItem *child)
{
    if (childItems.contains(child))
    {
        childItems.removeAll(child);
        delete child;
    }
}

int ChatRosterItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ChatRosterItem*>(this));

    return 0;
}

void ChatRosterItem::setData(int role, const QVariant &value)
{
    itemData.insert(role, value);
}

int ChatRosterItem::size() const
{
    return childItems.size();
}

enum ChatRosterItem::Type ChatRosterItem::type() const
{
    return itemType;
}
