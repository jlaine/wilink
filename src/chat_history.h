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
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QTextCursor>
#include <QTextDocument>

class QGraphicsLinearLayout;
class QUrl;

class ChatHistoryMessage
{
public:
    ChatHistoryMessage();

    bool archived;
    QString body;
    QDateTime date;
    QString from;
    QString fromJid;
    bool received;
};

class ChatSearchBubble : public QObject, public QGraphicsPathItem
{
    Q_OBJECT
    Q_PROPERTY(int margin READ margin WRITE setMargin)
 
public:
    ChatSearchBubble();
    int margin() const;
    void setMargin(int margin);
    void setSelection(const QRectF &selection);

private:
    int m_margin;
    QRectF m_selection;
};
 
class ChatTextItem : public QGraphicsTextItem
{
    Q_OBJECT

signals:
    void clicked();
    void linkHoverChanged(const QString &link);

protected:
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QString lastAnchor;
};

class ChatMessageWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatMessageWidget(bool local, QGraphicsItem *parent);
    bool collidesWithPath(const QPainterPath & path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    QTextCursor find(const QString &needle, const QTextCursor &cursor, QTextDocument::FindFlags flags);
    ChatHistoryMessage message() const;

    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    void setMessage(const ChatHistoryMessage &message);
    void setPrevious(ChatMessageWidget *previous);
    QList<QRectF> selection(const QTextCursor &cursor) const;
    void setSelection(const QRectF &rect);
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

signals:
    void messageClicked(const ChatHistoryMessage &message);
    void linkHoverChanged(const QString &link);

private slots:
    void slotTextClicked();

private:
    QPainterPath bodyPath(qreal width, qreal height, bool close);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;
    void setShowDate(bool show);
    bool showFooter();
    void setShowFooter(bool show);
    void setShowSender(bool show);

    int maxWidth;
    ChatHistoryMessage msg;
    bool show_date;
    bool show_sender;
    bool show_footer;

    // Text
    ChatTextItem *bodyText;
    QGraphicsTextItem *dateText;
    QGraphicsTextItem *fromText;

    // Graphics
    QGraphicsPathItem *messageBackground;
    QGraphicsPathItem *messageFrame;
    QGraphicsRectItem *messageShadow;
};

class ChatHistory : public QGraphicsView
{
    Q_OBJECT

public:
    ChatHistory(QWidget *parent = NULL);
    void addMessage(const ChatHistoryMessage &message);

public slots:
    void clear();
    void copy();
    void find(const QString &needle, QTextDocument::FindFlags flags);
    void selectAll();

signals:
    void findFinished(bool found);
    void focused();
    void messageClicked(const ChatHistoryMessage &message);

private slots:
    void slotLinkHoverChanged(const QString &link);
    void slotSelectionChanged();

protected:
    void adjustSize();
    qreal availableWidth() const;
    void contextMenuEvent(QContextMenuEvent *event);
    QString copyText();
    void focusInEvent(QFocusEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);

private:
    ChatSearchBubble *addSearchBubble(const QRectF &selection);
    void clearSearchBubbles();

    QGraphicsScene *scene;

    QList<ChatSearchBubble*> glassItems;
    QGraphicsWidget *obj;
    QGraphicsLinearLayout *layout;
    ChatMessageWidget *lastFindWidget;
    QTextCursor lastFindCursor;
    QList<ChatMessageWidget*> lastSelection;
    QString lastText;
};

#endif
