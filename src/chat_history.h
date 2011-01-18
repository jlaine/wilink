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
    bool groupWith(const ChatMessage &other) const;
    QString html() const;
    bool isAction() const;

    static void addTransform(const QRegExp &match, const QString &replacement);

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
    Q_OBJECT

public:
    ChatMessageBubble(QGraphicsItem *parent = 0);
    int indexOf(ChatMessageWidget *widget) const;
    void insertAt(int pos, ChatMessageWidget *widget);

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    ChatMessageBubble *splitAfter(ChatMessageWidget *widget);

signals:
    void messageClicked(const ChatMessage &message);

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private slots:
    void messageDestroyed(QObject *obj);

private:
    QGraphicsPathItem *m_frame;
    QGraphicsTextItem *m_from;

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
    ChatMessageWidget(const ChatMessage &message, QGraphicsItem *parent);
    bool collidesWithPath(const QPainterPath & path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    ChatMessage message() const;
    ChatMessageBubble *bubble();
    void setBubble(ChatMessageBubble *bubble);

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    void setPrevious(ChatMessageWidget *previous);
    QList<RectCursor> chunkSelection(const QTextCursor &cursor) const;
    void setSelection(const QRectF &rect);
    QGraphicsTextItem *textItem();

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    int maxWidth;
    ChatMessageBubble *m_bubble;
    ChatMessage m_message;

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
    void addMessage(const ChatMessage &message);
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
    void bubbleDestroyed(QObject *obj);
    void messageDestroyed(QObject *obj);
    void slotScrollChanged();
    void slotSelectionChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    void insertBubble(int pos, ChatMessageBubble *bubble);
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
