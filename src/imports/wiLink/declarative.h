/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#ifndef __WILINK_DECLARATIVE_H__
#define __WILINK_DECLARATIVE_H__

#include <QAbstractItemModel>
#include <QFileDialog>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QSortFilterProxyModel>

#include "QXmppMessage.h"
#include "QXmppPresence.h"

class QDeclarativeSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)

public:
    QDeclarativeSortFilterProxyModel(QObject *parent = 0);
    void setSourceModel(QAbstractItemModel *model);

public slots:
    void sort(int column);

signals:
    void sourceModelChanged(QAbstractItemModel *sourceModel);
};

class ListHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
    ListHelper(QObject *parent = 0);

    int count() const;
    Q_INVOKABLE QVariant get(int row) const;
    Q_INVOKABLE QVariant getProperty(int row, const QString &name) const;

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

signals:
    void countChanged();
    void modelChanged(QAbstractItemModel *model);

private:
    QAbstractItemModel *m_model;
    QList<QPersistentModelIndex> m_selection;
};

class Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri);
    void registerTypes(const char *uri);
};

#endif
