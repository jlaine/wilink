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

#ifndef __WILINK_SHARES_VIEW_H__
#define __WILINK_SHARES_VIEW_H__

#include <QStyledItemDelegate>
#include <QTreeView>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"

class ChatRosterModel;
class ChatSharesDatabase;
class ChatSharesModel;

class ChatSharesDelegate : public QStyledItemDelegate
{
public:
    ChatSharesDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

/** View for displaying a tree of share items.
 */
class ChatSharesView : public QTreeView
{
    Q_OBJECT

public:
    ChatSharesView(QWidget *parent = 0);
    void setModel(QAbstractItemModel *model);

signals:
    void contextMenu(const QModelIndex &index, const QPoint &globalPos);
    void expandRequested(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *e);
};

#endif
