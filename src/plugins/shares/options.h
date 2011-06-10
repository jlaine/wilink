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

#ifndef __WILINK_SHARES_OPTIONS_H__
#define __WILINK_SHARES_OPTIONS_H__

#include <QAbstractProxyModel>
#include <QFileSystemModel>

#include "plugins/preferences.h"

class QLineEdit;
class QTreeView;
class QXmppShareDatabase;

class FolderModel : public QFileSystemModel
{
    Q_OBJECT

public:
    FolderModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex & index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setForcedFolder(const QString &excluded);

    QStringList selectedFolders() const;
    void setSelectedFolders(const QStringList &selected);

private:
    QString m_forced;
    QStringList m_selected;
};

class PlaceModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    PlaceModel(QObject *parent = 0);
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    void setSourceModel(QFileSystemModel *sourceModel);

private slots:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
    QFileSystemModel *m_fsModel;
    QList<QString> m_paths;
};

/** View for displaying a tree of share items.
 */
class ShareOptions : public ChatPreferencesTab
{
    Q_OBJECT

public:
    ShareOptions(QXmppShareDatabase *database);
    bool save();

protected:
    void showEvent(QShowEvent *event);

private slots:
    void browse();
    void directorySelected(const QString &path);
    void fewerFolders();
    void moreFolders();
    void scrollToHome();

private:
    QPushButton *m_moreButton;
    QPushButton *m_fewerButton;
    QXmppShareDatabase *m_database;
    QLineEdit *m_directoryEdit;
    PlaceModel *m_placesModel;
    QTreeView *m_placesView;
    FolderModel *m_fsModel;
    QTreeView *m_fsView;
};

#endif
