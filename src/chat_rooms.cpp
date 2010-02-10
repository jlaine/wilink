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

#include "chat_rooms.h"

enum RoomsColumns {
    RoomColumn = 0,
    MaxColumn,
};

void ChatRoomsModel::addRoom(const QString &bareJid)
{
    if (roomKeys.contains(bareJid))
        return;
    beginInsertRows(QModelIndex(), roomKeys.size(), roomKeys.size());
    roomKeys.append(bareJid);
    endInsertRows();
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
    if (!index.isValid() || index.row() >= roomKeys.size())
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

int ChatRoomsModel::rowCount(const QModelIndex &parent) const
{
    return roomKeys.size();
}

ChatRoomsView::ChatRoomsView(ChatRoomsModel *model, QWidget *parent)
    : QTableView(parent)
{
    setModel(model);

    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotClicked()));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClicked()));

    setAlternatingRowColors(true);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setShowGrid(false);

    horizontalHeader()->setResizeMode(RoomColumn, QHeaderView::Stretch);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
}

void ChatRoomsView::slotClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit clicked(index.data(Qt::UserRole).toString());
}

void ChatRoomsView::slotDoubleClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit doubleClicked(index.data(Qt::UserRole).toString());
}

