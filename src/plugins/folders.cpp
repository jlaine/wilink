/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#include <QCoreApplication>
#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QDeclarativeExtensionPlugin>

#include "folders.h"

enum FolderRoles {
    IsDirRole = Qt::UserRole + 10,
    UrlRole
};

FolderModel::FolderModel(QObject *parent):
    QFileSystemModel(parent)
{
    // set role names
    QHash<int, QByteArray> names;
    names.insert(QFileSystemModel::FileNameRole, "name");
    names.insert(IsDirRole, "isDir");
    names.insert(UrlRole, "url");
    setRoleNames(names);
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    qDebug("data");
    if (role == IsDirRole) {
        return isDir(index);
    } else if (role == UrlRole) {
        return QUrl::fromLocalFile(filePath(index));
    } else {
        return QFileSystemModel::data(index, role);
    }
}

QUrl FolderModel::rootUrl() const
{
    return m_rootUrl;
}

void FolderModel::setRootUrl(const QUrl &rootUrl)
{
    if (rootUrl != m_rootUrl) {
        qDebug("set root url");
        m_rootUrl = rootUrl;
        setRootPath(rootUrl.toLocalFile());
        //refresh();
        emit rootUrlChanged(m_rootUrl);
    }
}

class FolderPlugin : public QDeclarativeExtensionPlugin
{
public:
    void registerTypes(const char *uri);
};

void FolderPlugin::registerTypes(const char *uri)
{
    qDebug("register2 %s", uri);
    qmlRegisterType<FolderModel>(uri, 2, 0, "FolderModel");
}

Q_EXPORT_PLUGIN2(qmlwilinkplugin, FolderPlugin);
