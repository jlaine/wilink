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

#include <QContextMenuEvent>
#include <QMenu>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppDiscoveryIq.h"

#include "chat_rooms.h"

enum RoomsColumns {
    RoomColumn = 0,
    MaxColumn,
};

ChatRoomsItem::ChatRoomsItem(const QString &id)
    : itemId(id), parentItem(0)
{
}

ChatRoomsItem::~ChatRoomsItem()
{
    foreach (ChatRoomsItem *item, childItems)
        delete item;
}

void ChatRoomsItem::append(ChatRoomsItem *item)
{
    if (item->parentItem)
    {
        qWarning("item already has a parent");
        return;
    }
    item->parentItem = this;
    childItems.append(item);
}

ChatRoomsItem *ChatRoomsItem::child(int row)
{
    if (row >= 0 && row < childItems.size())
        return childItems.at(row);
    else
        return 0;
}

bool ChatRoomsItem::contains(const QString &id) const
{
    foreach (const ChatRoomsItem *item, childItems)
    {
        if (item->id() == id)
            return true;
    }
    return false;
}

ChatRoomsItem *ChatRoomsItem::find(const QString &id)
{
    foreach (ChatRoomsItem *item, childItems)
        if (item->itemId == id)
            return item;
    return 0;
}

QString ChatRoomsItem::id() const
{
    return itemId;
}

ChatRoomsItem* ChatRoomsItem::parent()
{
    return parentItem;
}

void ChatRoomsItem::remove(ChatRoomsItem *child)
{
    if (childItems.contains(child))
    {
        childItems.removeAll(child);
        delete child;
    }
}

int ChatRoomsItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ChatRoomsItem*>(this));

    return 0;
}

int ChatRoomsItem::size() const
{
    return childItems.size();
}

ChatRoomsModel::ChatRoomsModel(QXmppClient *client)
    : xmppClient(client)
{
    rootItem = new ChatRoomsItem(NULL);
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
}

void ChatRoomsModel::addRoom(const QString &bareJid)
{
    if (rootItem->contains(bareJid))
        return;
    beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
    rootItem->append(new ChatRoomsItem(bareJid));
    endInsertRows();

#if 0
    // discover room info
    QXmppDiscoveryIq disco;
    disco.setTo(bareJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    xmppClient->sendPacket(disco);
#endif
}

int ChatRoomsModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatRoomsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ChatRoomsItem *item = static_cast<ChatRoomsItem*>(index.internalPointer());
    QString jid = item->id();
    if (role == Qt::UserRole) {
        return jid;
    } else if (role == Qt::DisplayRole) {
        if (jid.contains("/"))
            return jid.split("/")[1];
        else
            return roomName(jid);
    } else if (role == Qt::DecorationRole) {
        if (!jid.contains("/"))
            return QIcon(":/chat.png");
    }
    return QVariant();
}

void ChatRoomsModel::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
#if 0
    if (disco.getQueryType() == QXmppDiscoveryIq::ItemsQuery)
    {
        const QString bareJid = disco.getFrom().split("/")[0];
        roomParticipants[bareJid] = QStringList();
        foreach (const QXmppDiscoveryItem &item, disco.getItems())
            roomParticipants[bareJid].append(item.attribute("jid"));
    }
#endif
}

QModelIndex ChatRoomsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ChatRoomsItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ChatRoomsItem*>(parent.internalPointer());

    ChatRoomsItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ChatRoomsModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    ChatRoomsItem *childItem = static_cast<ChatRoomsItem*>(index.internalPointer());
    ChatRoomsItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

void ChatRoomsModel::presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.getFrom();
    const QString roomJid = jid.split("/")[0];
    ChatRoomsItem *roomItem = rootItem->find(roomJid);
    if (!roomItem)
        return;

    ChatRoomsItem *memberItem = roomItem->find(jid);
    if (presence.getType() == QXmppPresence::Available && !memberItem)
    {
        beginInsertRows(createIndex(roomItem->row(), 0, roomItem), roomItem->size(), roomItem->size());
        roomItem->append(new ChatRoomsItem(jid));
        endInsertRows();
    }
    else if (presence.getType() == QXmppPresence::Unavailable && memberItem)
    {
        beginRemoveRows(createIndex(roomItem->row(), 0, roomItem), memberItem->row(), memberItem->row());
        roomItem->remove(memberItem);
        endRemoveRows();
    }
}

void ChatRoomsModel::removeRoom(const QString &bareJid)
{
    ChatRoomsItem *roomItem = rootItem->find(bareJid);
    if (roomItem)
    {
        beginRemoveRows(QModelIndex(), roomItem->row(), roomItem->row());
        rootItem->remove(roomItem);
        endRemoveRows();
    }
}

QString ChatRoomsModel::roomName(const QString &bareJid) const
{
    return bareJid.split('@')[0];
}

int ChatRoomsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    ChatRoomsItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ChatRoomsItem*>(parent.internalPointer());

    return parentItem->size();
}

ChatRoomsView::ChatRoomsView(ChatRoomsModel *model, QWidget *parent)
    : QTreeView(parent)
{
    setModel(model);

    /* prepare context menu */
    QAction *action;
    contextMenu = new QMenu(this);
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Leave room"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotLeaveRoom()));

    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotClicked()));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClicked()));

    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void ChatRoomsView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    contextMenu->popup(event->globalPos());
}

void ChatRoomsView::selectRoom(const QString &jid)
{
    for (int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index = model()->index(i, 0);
        if (index.data(Qt::UserRole).toString() == jid)
        {
            setCurrentIndex(index);
            return;
        }
    }
    setCurrentIndex(QModelIndex());
}

QSize ChatRoomsView::sizeHint () const
{
    if (!model()->rowCount())
        return QTreeView::sizeHint();

    QSize hint(64, 0);
    hint.setHeight(model()->rowCount() * sizeHintForRow(0));
    for (int i = 0; i < MaxColumn; i++)
        hint.setWidth(hint.width() + sizeHintForColumn(i));
    return hint;
}

void ChatRoomsView::slotClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
    {
        const QString jid = index.data(Qt::UserRole).toString().split("/")[0];
        emit clicked(jid);
    }
}

void ChatRoomsView::slotDoubleClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
    {
        const QString jid = index.data(Qt::UserRole).toString().split("/")[0];
        emit doubleClicked(jid);
    }
}

void ChatRoomsView::slotLeaveRoom()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
    {
        const QString jid = index.data(Qt::UserRole).toString().split("/")[0];
        emit leaveRoom(jid);
    }
}

