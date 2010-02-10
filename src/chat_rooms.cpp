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

#include "chat_rooms.h"

QString ChatRoomsModel::roomName(const QString &bareJid) const
{
    return bareJid.split('@')[0];
}

int ChatRoomsModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ChatRoomsModel::data(const QModelIndex &index, int role) const
{

}

int ChatRoomsModel::rowCount(const QModelIndex &parent) const
{
    return roomKeys.size();
}

ChatRoomsView::ChatRoomsView(ChatRoomsModel *model, QWidget *parent)
    : QTableView(parent)
{
    setModel(model);
}

