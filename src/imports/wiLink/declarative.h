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

#ifndef __WILINK_DECLARATIVE_H__
#define __WILINK_DECLARATIVE_H__

#include <QAbstractItemModel>
#include <QDeclarativeItem>
#include <QDeclarativeExtensionPlugin>
#include <QFileDialog>
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

class DropArea : public QDeclarativeItem
{
    Q_OBJECT

public:
    DropArea(QDeclarativeItem *parent = 0);

signals:
    void filesDropped(const QStringList &files);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);
};

class WheelArea : public QDeclarativeItem
{
    Q_OBJECT

public:
    WheelArea(QDeclarativeItem *parent = 0);

protected:
    void wheelEvent(QGraphicsSceneWheelEvent *event);

signals:
    void verticalWheel(int delta);
    void horizontalWheel(int delta);
};

class Plugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void initializeEngine(QDeclarativeEngine *engine, const char *uri);
    void registerTypes(const char *uri);
};

#endif
