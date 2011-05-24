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

#ifndef __WILINK_CHAT_HISTORY_H__
#define __WILINK_CHAT_HISTORY_H__

#include <QDateTime>
#include <QGraphicsWidget>
#include <QTextCursor>
#include <QTextDocument>

#include "chat_model.h"

class QGraphicsLinearLayout;
class QGraphicsView;
class QUrl;

class ChatHistoryModel;
class ChatHistoryModelPrivate;
class ChatHistoryWidget;
class ChatRosterModel;
class ChatSearchBubble;
typedef QPair<QRectF, QTextCursor> RectCursor;

/** The ChatHistoryBubble class is a widget for display a set of
 *  chat messages from a given sender.
 */
class ChatHistoryBubble : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatHistoryBubble(ChatHistoryWidget *parent);
    QModelIndex index() const;
    ChatHistoryModel *model();
    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    QGraphicsTextItem *textItem();

public slots:
    void dataChanged();

signals:
    void messageClicked(const QModelIndex &index);

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    QGraphicsPixmapItem *m_avatar;
    QGraphicsTextItem *m_body;
    QString m_bodyAnchor;
    QGraphicsTextItem *m_date;
    QGraphicsPathItem *m_frame;
    QGraphicsTextItem *m_from;
    ChatHistoryWidget *m_history;

    // layout
    qreal m_maximumWidth;
};

/** The ChatHistoryWidget class represents a widget for displaying messages.
 */
class ChatHistoryWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatHistoryWidget(QGraphicsItem *parent = 0);
    int indexOf(ChatHistoryBubble *bubble);
    ChatHistoryModel *model();
    void setModel(ChatHistoryModel *model);
    void setView(QGraphicsView *view);

signals:
    void findFinished(bool found);
    void messageClicked(const QModelIndex &index);

public slots:
    void clear();
    void copy();
    void find(const QString &needle, QTextDocument::FindFlags flags, bool changed);
    void findClear();
    void resizeWidget();
    void selectAll();

private slots:
    void rowsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void slotScrollChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QGraphicsTextItem *textItemAt(const QPointF &pos) const;
    QString selectedText() const;

    // viewport
    bool m_followEnd;
    QGraphicsView *m_view;

    // layout
    QGraphicsLinearLayout *m_layout;
    qreal m_maximumWidth;
    QList<ChatHistoryBubble*> m_bubbles;
    ChatHistoryModel *m_model;
    QTimer *m_resizeTimer;

    // selection
    QList<QGraphicsTextItem*> m_selectedMessages;
    QRectF m_selectionRectangle;
    QPointF m_selectionStart;
    QTimer *m_trippleClickTimer;

    // search
    QList<ChatSearchBubble*> m_glassItems;
    QGraphicsTextItem *m_lastFindWidget;
    QTextCursor m_lastFindCursor;
};

class ChatSearchBubble : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)

public:
    ChatSearchBubble();
    void bounce();
    QRectF boundingRect() const;
    void setSelection(const RectCursor &selection);

private:
    QGraphicsPathItem *bubble;
    QGraphicsPathItem *shadow;
    QGraphicsTextItem *text;
    const int m_margin;
    const int m_radius;
    RectCursor m_selection;
};

#endif
