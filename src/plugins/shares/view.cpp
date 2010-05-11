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

#include "database.h"
#include "model.h"
#include "view.h"

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 100

ChatSharesDelegate::ChatSharesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ChatSharesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int error = index.data(TransferError).toInt();
    int done = index.data(TransferDone).toInt();
    int total = index.data(TransferTotal).toInt();
    QString localPath = index.data(TransferPath).toString();
    if (index.column() == ProgressColumn && done > 0 && total > 0 && !error && localPath.isEmpty())
    {
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = total; 
        progressBarOption.progress = done;
        progressBarOption.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_Selected;
        progressBarOption.text = index.data(Qt::DisplayRole).toString();
        progressBarOption.textVisible = true;

        QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                           &progressBarOption, painter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeView(parent)
{
    setItemDelegate(new ChatSharesDelegate(this));
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void ChatSharesView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    emit contextMenu(index, event->globalPos());
}

void ChatSharesView::keyPressEvent(QKeyEvent *event)
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
void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    int nameWidth =  e->size().width();
    if (!isColumnHidden(SizeColumn))
        nameWidth -= SIZE_COLUMN_WIDTH;
    if (!isColumnHidden(ProgressColumn))
        nameWidth -= PROGRESS_COLUMN_WIDTH;
    setColumnWidth(NameColumn, nameWidth);
}

void ChatSharesView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(SizeColumn, SIZE_COLUMN_WIDTH);
    setColumnWidth(ProgressColumn, PROGRESS_COLUMN_WIDTH);
}

