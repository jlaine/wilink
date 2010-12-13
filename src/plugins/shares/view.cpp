/*
 * wiLink
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

#include <QApplication>
#include <QResizeEvent>
#include <QStringList>
#include <QUrl>

#include "model.h"
#include "view.h"

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 100

SharesDelegate::SharesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SharesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int error = index.data(TransferError).toInt();
    qint64 done = index.data(TransferDone).toLongLong();
    qint64 total = index.data(TransferTotal).toLongLong();
    QString localPath = index.data(TransferPath).toString();
    if (index.column() == ProgressColumn &&
        done > 0 && total > 0 && done <= total &&
        !error && localPath.isEmpty())
    {
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = (100 * done) / total;
        progressBarOption.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_Selected;
        progressBarOption.text = index.data(Qt::DisplayRole).toString();
        progressBarOption.textVisible = true;

        QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                           &progressBarOption, painter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

SharesSelectionModel::SharesSelectionModel(QAbstractItemModel *model, QObject *parent)
    : QItemSelectionModel(model, parent)
{
}

/** Reimplemented to never select both an item and one of its children.
 */
void SharesSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    if (command & QItemSelectionModel::Select)
    {
        bool isClear = (command & QItemSelectionModel::Clear);

        // select the requested items, filtering out items whose parent
        // is already selected (or about to be)
        QItemSelection addSelection;
        foreach (const QModelIndex &index, selection.indexes())
        {
            // if a parent of this item is selected (or about to be),
            // skip the item
            bool skip = false;
            QModelIndex parent = index.parent();
            while (parent.isValid())
            {
                if (selection.contains(parent) || (!isClear && isSelected(parent)))
                {
                    skip = true;
                    break;
                }
                parent = parent.parent();
            }
            if (!skip)
                addSelection.append(QItemSelectionRange(index));
        }
        QItemSelectionModel::select(addSelection, command);

        // if a clear was requested, stop here
        if (isClear)
            return;

        // deselect any children of the selected items
        QItemSelection removeSelection;
        foreach (const QModelIndex &index, selectedRows())
        {
            QModelIndex parent = index.parent();
            while (parent.isValid())
            {
                if (selection.contains(parent))
                {
                    removeSelection.append(QItemSelectionRange(index));
                    break;
                }
                parent = parent.parent();
            }
        }
        QItemSelectionModel::select(removeSelection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    } else {
        QItemSelectionModel::select(selection, command);
    }
}

SharesView::SharesView(QWidget *parent)
    : QTreeView(parent)
{
    setItemDelegate(new SharesDelegate(this));
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void SharesView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    emit contextMenu(index, event->globalPos());
}

void SharesView::keyPressEvent(QKeyEvent *event)
{
    QModelIndex current = currentIndex();
    if (current.isValid())
    {
        switch (event->key())
        {
        case Qt::Key_Plus:
        case Qt::Key_Right:
            emit expandRequested(current);
            return;
        case Qt::Key_Minus:
        case Qt::Key_Left:
            collapse(current);
            return;
        }
    }
    QTreeView::keyPressEvent(event);
}

/** When the view is resized, adjust the width of the
 *  "name" column to take all the available space.
 */
void SharesView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    int nameWidth =  e->size().width();
    if (!isColumnHidden(SizeColumn))
        nameWidth -= SIZE_COLUMN_WIDTH;
    if (!isColumnHidden(ProgressColumn))
        nameWidth -= PROGRESS_COLUMN_WIDTH;
    setColumnWidth(NameColumn, nameWidth);
}

void SharesView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(SizeColumn, SIZE_COLUMN_WIDTH);
    setColumnWidth(ProgressColumn, PROGRESS_COLUMN_WIDTH);
    setSelectionModel(new SharesSelectionModel(model, this));
}

