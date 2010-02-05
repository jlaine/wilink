/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_HISTORY_H__
#define __WDESKTOP_CHAT_HISTORY_H__

#include <QGraphicsView>
#include <QGraphicsWidget>

#include "qxmpp/QXmppArchiveIq.h"

class QGraphicsLinearLayout;

class ChatTextItem : public QGraphicsTextItem
{
    Q_OBJECT

signals:
    void linkHoverChanged(const QString &link);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    QString lastAnchor;
};

class ChatMessageWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatMessageWidget(bool local, QGraphicsItem *parent);
    bool collidesWithPath(const QPainterPath & path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    bool archived() const;
    QString from() const;
    QXmppArchiveMessage message() const;

    void setArchived(bool archived);
    void setFrom(const QString &from);
    void setGeometry(const QRectF &rect);
    void setMaximumWidth(qreal width);
    void setMessage(const QXmppArchiveMessage &message);
    void setSelected(bool selected);
    void setShowDate(bool show);
    void setShowSender(bool show);

signals:
    void linkHoverChanged(const QString &link);

protected:
    QPainterPath bubblePath(qreal width);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

private:
    int maxWidth;
    QXmppArchiveMessage msg;
    bool msgArchived;
    QString msgFrom;
    bool show_date;
    bool show_sender;

    ChatTextItem *bodyText;
    QGraphicsPathItem *dateBubble;
    QGraphicsLineItem *dateLine;
    QGraphicsTextItem *dateText;
    QGraphicsTextItem *fromText;
};

class ChatHistory : public QGraphicsView
{
    Q_OBJECT

public:
    ChatHistory(QWidget *parent = NULL);
    void addMessage(const QXmppArchiveMessage &message, bool archived = false);
    void setLocalName(const QString &localName);
    void setRemoteName(const QString &remoteName);

public slots:
    void clear();
    void copy();

protected slots:
    void slotLinkHoverChanged(const QString &link);
    void slotSelectionChanged();

protected:
    void adjustSize();
    qreal availableWidth() const;
    void contextMenuEvent(QContextMenuEvent *event);
    QString copyText();
    void resizeEvent(QResizeEvent *e);

private:
    QString chatLocalName;
    QString chatRemoteName;
    QGraphicsScene *scene;

    QGraphicsWidget *obj;
    QGraphicsLinearLayout *layout;
    QList<QGraphicsItem*> lastSelection;
};

#endif
