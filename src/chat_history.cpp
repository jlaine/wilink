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

#include "chat_history.h"

#include <QDebug>
#include <QGraphicsLinearLayout>

ChatMessageWidget::ChatMessageWidget(const QXmppArchiveMessage &message, QGraphicsItem *parent)
        : QGraphicsWidget(parent)
{
    bodyText = scene()->addText(message.body);
    bodyText->setParentItem(this);

    QDateTime datetime = message.datetime.toLocalTime();
    QString dateString(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
    dateText = scene()->addText(dateString);
    dateText->setParentItem(this);

    fromText = scene()->addText("Foo");
    fromText->setParentItem(this);
}

void ChatMessageWidget::setGeometry(const QRectF &rect)
{
    fromText->setPos(rect.x(), rect.y());

    dateText->setPos(rect.x() + rect.width() - 80, rect.y());
    dateText->setTextWidth(80);

    bodyText->setPos(rect.x(), rect.y() + 20);
    bodyText->setTextWidth(rect.width());
}

QSizeF ChatMessageWidget::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    QSizeF size;
    size.setWidth(200);
    size.setHeight(50);
    return size;
}

ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene;
    setScene(scene);

    obj = new QGraphicsWidget;
    layout = new QGraphicsLinearLayout(Qt::Vertical);
    obj->setLayout(layout);
    scene->addItem(obj);
}

void ChatHistory::addMessage(const QXmppArchiveMessage &message)
{
    ChatMessageWidget *msg = new ChatMessageWidget(message, obj) ;
    layout->addItem(msg);
}

void ChatHistory::setLocalName(const QString &localName)
{
    chatLocalName = localName;
}

void ChatHistory::setRemoteName(const QString &remoteName)
{
    chatRemoteName = remoteName;
}

