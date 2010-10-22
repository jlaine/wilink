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

#ifndef __WILINK_CHAT_HISTORY_H__
#define __WILINK_CHAT_HISTORY_H__

#include <QDateTime>
#include <QGraphicsWidget>
#include <QTextCursor>
#include <QTextDocument>

class QGraphicsLinearLayout;
class QGraphicsView;
class QUrl;

class ChatMessageWidget;
class ChatSearchBubble;
typedef QPair<QRectF, QTextCursor> RectCursor;

/** The ChatMessage class represents the data for a single chat history message.
 */
class ChatMessage
{
public:
    ChatMessage();

    bool archived;
    QString body;
    QDateTime date;
    QString from;
    QString fromJid;
    bool received;
};

/** The ChatMessageBubble class is a widget for display a set of
 *  chat messages from a given sender.
 */
class ChatMessageBubble : public QGraphicsWidget
{
public:
    ChatMessageBubble(bool received, QGraphicsItem *parent = 0);
    int indexOf(ChatMessageWidget *widget) const;
    void insertAt(int pos, ChatMessageWidget *widget);

    QString from() const;
    void setFrom(const QString &from);

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    QGraphicsPathItem *m_frame;
    QGraphicsTextItem *m_from;
    QGraphicsRectItem *m_shadow;

    // layout
    qreal m_maximumWidth;
    QList<ChatMessageWidget*> m_messages;
};

/** The ChatMessageWidget class represents a widget for displaying a single
 *  chat message.
 */
class ChatMessageWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatMessageWidget(bool local, QGraphicsItem *parent);
    bool collidesWithPath(const QPainterPath & path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    ChatMessage message() const;
    ChatMessageBubble *bubble();
    void setBubble(ChatMessageBubble *bubble);

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    void setMessage(const ChatMessage &message);
    void setPrevious(ChatMessageWidget *previous);
    QList<RectCursor> chunkSelection(const QTextCursor &cursor) const;
    void setSelection(const QRectF &rect);
    QTextDocument *document() const;
    QGraphicsTextItem *textItem();

signals:
    void messageClicked(const ChatMessage &message);

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);

private:
    QPainterPath bodyPath(qreal width, qreal height, bool close);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;
    void setShowDate(bool show);

    int maxWidth;
    ChatMessage msg;
    bool show_date;

    ChatMessageBubble *m_bubble;

    // Text
    QString bodyAnchor;
    QGraphicsTextItem *bodyText;
    QGraphicsTextItem *dateText;
};

/** The ChatHistoryWidget class represents a widget containing a list
 *  of ChatMessageWidget.
 */
class ChatHistoryWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    ChatHistoryWidget(QGraphicsItem *parent = 0);
    ChatMessageWidget *addMessage(const ChatMessage &message);
    void adjustSize();
    void setView(QGraphicsView *view);

signals:
    void findFinished(bool found);
    void messageClicked(const ChatMessage &message);

public slots:
    void clear();
    void copy();
    void find(const QString &needle, QTextDocument::FindFlags flags, bool changed);
    void findClear();
    void selectAll();

private slots:
    void slotScrollChanged();
    void slotSelectionChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    ChatMessageWidget *messageWidgetAt(const QPointF &pos) const;
    QString selectedText() const;

    // viewport
    bool m_followEnd;
    QGraphicsView *m_view;

    // layout
    QGraphicsLinearLayout *m_layout;
    qreal m_maximumWidth;
    QList<ChatMessageBubble*> m_bubbles;
    QList<ChatMessageWidget*> m_messages;

    // selection
    QList<ChatMessageWidget*> m_selectedMessages;
    QRectF m_selectionRectangle;
    QPointF m_selectionStart;
    QTimer *m_trippleClickTimer;

    // search
    QList<ChatSearchBubble*> m_glassItems;
    ChatMessageWidget *m_lastFindWidget;
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
