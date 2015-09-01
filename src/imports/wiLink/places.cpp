/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

#include <QDir>
#include <QUrl>
#include <QStandardPaths>

#include "model.h"
#include "places.h"

PlaceModel::PlaceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QList<QStandardPaths::StandardLocation> locations;
    locations << QStandardPaths::DocumentsLocation;
    locations << QStandardPaths::MusicLocation;
    locations << QStandardPaths::MoviesLocation;
    locations << QStandardPaths::PicturesLocation;

    foreach (QStandardPaths::StandardLocation location, locations) {
        const QStringList paths = QStandardPaths::standardLocations(location);
        foreach (const QString &path, paths) {
            QDir dir(path);
            if (path.isEmpty() || dir == QDir::home())
                continue;

            // do not use "path" directly, on Windows it uses back slashes
            // where the rest of Qt uses forward slashes
            m_paths << dir.path();
        }
    }
}

QVariant PlaceModel::data(const QModelIndex &index, int role) const
{
    const int i = index.row();
    if (!index.isValid())
        return QVariant();

    if (role == ChatModel::AvatarRole)
        return QUrl("image://icon/album");
    else if (role == ChatModel::NameRole)
        return QFileInfo(m_paths.value(i)).fileName();
    else if (role == ChatModel::JidRole)
        return QUrl::fromLocalFile(m_paths.value(i));

    return QVariant();
}

QHash<int, QByteArray> PlaceModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ChatModel::AvatarRole, "avatar");
    roleNames.insert(ChatModel::NameRole, "name");
    roleNames.insert(ChatModel::JidRole, "url");
    return roleNames;
}

int PlaceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else
        return m_paths.size();
}


