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
#include <QDesktopServices>
#include <QFile>
#include <QGraphicsLinearLayout>
#include <QMenu>
#include <QScrollBar>

#include <QTextBlock>

#define DATE_WIDTH 80
#define DATE_HEIGHT 12

ChatMessageWidget::ChatMessageWidget(bool local, QGraphicsItem *parent)
    : QGraphicsWidget(parent), show_date(true), show_sender(false), maxWidth(2 * DATE_WIDTH)
{
    bodyText = scene()->addText("");
    bodyText->setParentItem(this);
    bodyText->setOpenExternalLinks(true);
    bodyText->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QColor baseColor = local ? QColor(0xdb, 0xdb, 0xdb) : QColor(0xb6, 0xd4, 0xff);
    QLinearGradient grad(QPointF(0, 0), QPointF(0, 0.5));
    grad.setColorAt(0, baseColor);
    baseColor.setAlpha(0x80);
    grad.setColorAt(1, baseColor);
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad.setSpread(QGradient::ReflectSpread);

    dateBubble = scene()->addPath(bubblePath(DATE_WIDTH), QPen(Qt::gray), QBrush(grad));
    dateBubble->setParentItem(this);
    dateBubble->setZValue(-1);

    dateLine = scene()->addLine(0, 0, DATE_WIDTH, 0, QPen(Qt::gray));
    dateLine->setParentItem(this);

    dateText = scene()->addText("");
    QFont font = dateText->font();
    font.setPixelSize(10);
    dateText->setFont(font);
    dateText->setParentItem(this);
    dateText->setTextWidth(90);

    fromText = scene()->addText("");
    fromText->hide();
    fromText->setFont(font);
    fromText->setParentItem(this);
}

QPainterPath ChatMessageWidget::bubblePath(qreal width)
{
    QPainterPath path;
    path.moveTo(DATE_HEIGHT/2, 0);
    path.arcTo(0, 0, DATE_HEIGHT, DATE_HEIGHT, 90, 180);
    path.lineTo(width - DATE_HEIGHT/2, DATE_HEIGHT);
    path.arcTo(width - DATE_HEIGHT, 0, DATE_HEIGHT, DATE_HEIGHT, -90, 180);
    path.closeSubpath();
    return path;
}

void ChatMessageWidget::setBody(const QString &body)
{
    QString bodyHtml = Qt::escape(body);
    bodyHtml.replace("\n", "<br/>");
    bodyHtml.replace(QRegExp("((ftp|http|https)://[^ ]+)"), "<a href=\"\\1\">\\1</a>");
    bodyText->setHtml(bodyHtml);
}

void ChatMessageWidget::setDate(const QDateTime &datetime)
{
    dateText->setPlainText(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
}

void ChatMessageWidget::setFrom(const QString &from)
{
    fromText->setPlainText(from);
}

void ChatMessageWidget::setGeometry(const QRectF &baseRect)
{
    QRectF rect(baseRect);
    if (!show_sender)
        rect.moveTop(rect.y() - DATE_HEIGHT/2);
    if (show_sender)
    {
        dateBubble->setPath(bubblePath(rect.width()));
        dateBubble->setPos(rect.x(), rect.y() + 0.5);
        fromText->setPos(rect.x() + 10, rect.y() - 4);
        bodyText->setPos(rect.x(), rect.y() + DATE_HEIGHT);
    } else {
        if (show_date)
        {
            dateBubble->setPos(rect.right() - DATE_WIDTH, rect.y() + 0.5);
            dateLine->setLine(0, 0, rect.width() - DATE_WIDTH, 0);
        } else {
            dateLine->setLine(0, 0, rect.width(), 0);
        }
        dateLine->setPos(rect.x(), rect.y() + DATE_HEIGHT/2 + 0.5);
        bodyText->setPos(rect.x(), rect.y() + DATE_HEIGHT/2 + 0.5);
    }
    dateText->setPos(
        rect.right() - (DATE_WIDTH + dateText->document()->idealWidth())/2,
        rect.y() - 4);
}

void ChatMessageWidget::setMaximumWidth(qreal width)
{
    QGraphicsWidget::setMaximumWidth(width);
    maxWidth = width;
    bodyText->document()->setTextWidth(width - DATE_WIDTH);
    updateGeometry();
}

void ChatMessageWidget::setShowDate(bool show)
{
    if (show)
    {
        dateBubble->show();
        dateText->show();
    } else {
        dateBubble->hide();
        dateText->hide();
    }
    show_date = show;
}

void ChatMessageWidget::setShowSender(bool show)
{
    if (show)
    {
        dateLine->hide();
        fromText->show();
    } else {
        dateLine->show();
        fromText->hide();
    }
    show_sender = show;
}

QSizeF ChatMessageWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
            return QSizeF(DATE_WIDTH, DATE_HEIGHT);
        case Qt::PreferredSize:
        {
            QSizeF hint(bodyText->document()->size());
            if (hint.width() < maxWidth)
                hint.setWidth(maxWidth);
            if (show_sender)
                hint.setHeight(hint.height() + DATE_HEIGHT);
            return hint;
        }
        default:
            return constraint;
    }
}

ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene;
    setScene(scene);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setRenderHints(QPainter::Antialiasing);

    obj = new QGraphicsWidget;
    layout = new QGraphicsLinearLayout(Qt::Vertical);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    obj->setLayout(layout);
    scene->addItem(obj);
}

void ChatHistory::addMessage(const QXmppArchiveMessage &message, bool archived)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() == scrollBar->maximum();

    /* check for archived message collisions */
    if (archived)
    {
        foreach (const QXmppArchiveMessage &existing, messages)
        {
            if (message.datetime == existing.datetime &&
                message.local == existing.local &&
                message.body == existing.body)
                return;
        }
    }

    /* position cursor */
    int i = 0;
    while (i < messages.size() && message.datetime > messages.at(i).datetime)
        i++;
    messages.insert(i, message);

    /* determine grouping */
    bool showSender = (i == 0 ||
        message.local != messages.at(i-1).local ||
        message.datetime > messages.at(i-1).datetime.addSecs(120 * 60));
    bool showDate = (showSender ||
        message.datetime > messages.at(i-1).datetime.addSecs(60));

    /* add message */
    ChatMessageWidget *msg = new ChatMessageWidget(message.local, obj);
    msg->setBody(message.body);
    msg->setDate(message.datetime.toLocalTime());
    msg->setFrom(message.local ? chatLocalName : chatRemoteName);
    msg->setShowDate(showDate);
    msg->setShowSender(showSender);

    msg->setMaximumWidth(availableWidth());
    layout->insertItem(i, msg);
    adjustSize();
    // FIXME: for some reason, we need to call updateGeometry() on message
    msg->setMaximumWidth(availableWidth());

    /* scroll to end if we were previous at end */
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::adjustSize()
{
    obj->adjustSize();
    QRectF rect = obj->boundingRect();
    rect.setHeight(rect.height() - 10);
    setSceneRect(rect);
}

qreal ChatHistory::availableWidth() const
{
    return width() - verticalScrollBar()->sizeHint().width() - 10;
}

void ChatHistory::clear()
{
    messages.clear();
    for (int i = layout->count() - 1; i >= 0; i--)
        delete layout->itemAt(i);
    adjustSize();
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu;
    QAction *action = menu->addAction(QIcon(":/remove.png"), tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(clear()));
    menu->exec(event->globalPos());
    delete menu;
}

void ChatHistory::resizeEvent(QResizeEvent *e)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() == scrollBar->maximum();

    const qreal w = availableWidth();
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));
        child->setMaximumWidth(w);
    }
    adjustSize();
    //QGraphicsView::resizeEvent(e);

    /* scroll to end if we were previous at end */
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::setLocalName(const QString &localName)
{
    chatLocalName = localName;
}

void ChatHistory::setRemoteName(const QString &remoteName)
{
    chatRemoteName = remoteName;
}

void ChatHistory::slotAnchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}

