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
class ChatMessageWidget;
class ChatRosterModel;
class ChatSearchBubble;
typedef QPair<QRectF, QTextCursor> RectCursor;

/** The ChatMessage class represents the data for a single chat history message.
 */
class ChatMessage
{
public:
    ChatMessage();
    bool groupWith(const ChatMessage &other) const;
    bool isAction() const;

    static void addTransform(const QRegExp &match, const QString &replacement);

    bool archived;
    QString body;
    QDateTime date;
    QString jid;
    bool received;
};

/** The ChatMessageBubble class is a widget for display a set of
 *  chat messages from a given sender.
 */
class ChatMessageBubble : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatMessageBubble(ChatHistoryWidget *parent);
    QModelIndex index() const;
    int indexOf(ChatMessageWidget *widget) const;
    void insertAt(int pos, ChatMessageWidget *widget);
    ChatHistoryModel *model();
    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    ChatMessageWidget *takeAt(int pos);

public slots:
    void dataChanged();

signals:
    void messageClicked(const QModelIndex &index);

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    ChatHistoryWidget *m_history;
    QGraphicsPathItem *m_frame;
    QGraphicsTextItem *m_from;

    // layout
    qreal m_maximumWidth;
    QList<ChatMessageWidget*> m_messages;

    friend class ChatHistoryWidget;
};

/** The ChatMessageWidget class represents a widget for displaying a single
 *  chat message.
 */
class ChatMessageWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatMessageWidget(QGraphicsItem *parent);
    QModelIndex index() const;
    ChatMessageBubble *bubble();
    void setBubble(ChatMessageBubble *bubble);

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    QGraphicsTextItem *textItem();

public slots:
    void dataChanged();

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    int maxWidth;
    ChatMessageBubble *m_bubble;

    // Text
    QString bodyAnchor;
    QGraphicsTextItem *bodyText;
    QGraphicsTextItem *dateText;
};

class ChatHistoryModel : public ChatModel
{
    Q_OBJECT

public:
    enum HistoryRole {
        ActionRole = Qt::UserRole,
        AvatarRole,
        BodyRole,
        DateRole,
        FromRole,
        JidRole,
        HtmlRole,
        ReceivedRole,
    };

    ChatHistoryModel(QObject *parent = 0);
    void addMessage(const ChatMessage &message);
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    ChatRosterModel *rosterModel();
    void setRosterModel(ChatRosterModel *rosterModel);

public slots:
    void clear();

private:
    ChatHistoryModelPrivate *d;
};

/** The ChatHistoryWidget class represents a widget containing a list
 *  of ChatMessageWidget.
 */
class ChatHistoryWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    ChatHistoryWidget(QGraphicsItem *parent = 0);
    void adjustSize();
    int indexOf(ChatMessageBubble *bubble);
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
    void selectAll();

private slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int destRow);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void slotScrollChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    void insertBubble(int pos, ChatMessageBubble *bubble);
    QGraphicsTextItem *textItemAt(const QPointF &pos) const;
    QString selectedText() const;

    // viewport
    bool m_followEnd;
    QGraphicsView *m_view;

    // layout
    QGraphicsLinearLayout *m_layout;
    qreal m_maximumWidth;
    QList<ChatMessageBubble*> m_bubbles;
    ChatHistoryModel *m_model;

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
