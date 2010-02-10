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

#include <QHeaderView>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppDiscoveryIq.h"

#include "chat_rooms.h"

enum RoomsColumns {
    RoomColumn = 0,
    MaxColumn,
};

ChatRoomsModel::ChatRoomsModel(QXmppClient *client)
    : xmppClient(client)
{
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
}

void ChatRoomsModel::addRoom(const QString &bareJid)
{
    if (roomKeys.contains(bareJid))
        return;
    beginInsertRows(QModelIndex(), roomKeys.size(), roomKeys.size());
    roomKeys.append(bareJid);
    roomParticipants[bareJid] = QStringList();
    endInsertRows();

#if 0
    // discover room info
    QXmppDiscoveryIq disco;
    disco.setTo(bareJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    xmppClient->sendPacket(disco);
#endif
}

QString ChatRoomsModel::roomName(const QString &bareJid) const
{
    return bareJid.split('@')[0];
}

int ChatRoomsModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatRoomsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.internalId() > 0)
    {
        int id = index.internalId() - 1;
        if (id >= roomKeys.size())
            return QVariant();
        const QString roomJid = roomKeys.at(id);

        if (index.row() >= roomParticipants[roomJid].size())
            return QVariant();
        const QString jid = roomParticipants[roomJid].at(index.row());

        if (role == Qt::UserRole) {
            return jid;
        } else if (role == Qt::DisplayRole) {
            return jid.split("/")[1];
        }
    } else {
        if (index.row() >= roomKeys.size())
            return QVariant();

        const QString bareJid = roomKeys.at(index.row());
        if (role == Qt::UserRole) {
            return bareJid;
        } else if (role == Qt::DisplayRole) {
            return roomName(bareJid);
        } else if (role == Qt::DecorationRole) {
            return QIcon(":/chat.png");
        }
    }
    return QVariant();
}

void ChatRoomsModel::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    qDebug() << "discovery received";
    foreach (const QXmppDiscoveryItem &item, disco.getItems())
    {
        qDebug() << " *" << item.type();
        foreach (const QString &attr, item.attributes())
            qDebug() << "   -" << attr << ":" << item.attribute(attr);
    }
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

    if (!parent.isValid())
    {
        if (row < roomKeys.size())
            return createIndex(row, column, 0);
        else
            return QModelIndex();
    } else if (!parent.internalId()) {
        QString jid = roomKeys.at(parent.row());
        if (row < roomParticipants[jid].size())
            return createIndex(row, column, 1 + parent.row());
        else
            return QModelIndex();
    }
    return QModelIndex();
}

QModelIndex ChatRoomsModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    if (index.internalId() > 0 && index.internalId() <= roomKeys.size())
        return createIndex(index.internalId() - 1, index.column(), 0);
    return QModelIndex();
}

void ChatRoomsModel::presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.getFrom();
    const QString roomJid = jid.split("/")[0];
    int roomIndex = roomKeys.indexOf(roomJid);
    if (roomIndex < 0)
        return;

    if (presence.getType() == QXmppPresence::Available)
    {
        int index = roomParticipants[roomJid].indexOf(jid);
        if (index < 0)
        {
            beginInsertRows(createIndex(roomIndex, 0, 0), roomParticipants[roomJid].size(), roomParticipants[roomJid].size());
            roomParticipants[roomJid].append(jid);
            endInsertRows();
        }
    }
    else if (presence.getType() == QXmppPresence::Unavailable)
    {
        int index = roomParticipants[roomJid].indexOf(jid);
        if (index >= 0)
        {
            beginRemoveRows(createIndex(roomIndex, 0, 0), index, index);
            roomParticipants[roomJid].removeAt(index);
            endRemoveRows();
        }
    }
}

int ChatRoomsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
    {
        return roomKeys.size();
    } else if (!parent.internalId()) {
        const QString bareJid = roomKeys.at(parent.row());
        Q_ASSERT(roomParticipants.contains(bareJid));
        if (roomParticipants.contains(bareJid))
            return roomParticipants[bareJid].size();
    }
    return 0;
}

ChatRoomsView::ChatRoomsView(ChatRoomsModel *model, QWidget *parent)
    : QTreeView(parent)
{
    setModel(model);

    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotClicked()));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClicked()));

    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
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

