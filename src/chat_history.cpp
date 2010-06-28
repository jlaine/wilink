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

#include "chat_history.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include <QScrollBar>
#include <QShortcut>
#include <QTextDocument>
#include <QUrl>

#define DATE_WIDTH 80
#define FROM_HEIGHT 15
#define HEADER_HEIGHT 10
#define BODY_HEIGHT 15
#define BODY_OFFSET 5
#define FOOTER_HEIGHT 5
#define MESSAGE_WIDTH 200

void ChatTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!lastAnchor.isEmpty())
    {
        emit linkHoverChanged("");
        lastAnchor = "";
    }
    QGraphicsTextItem::hoverMoveEvent(event);
}

void ChatTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QString anchor = document()->documentLayout()->anchorAt(event->pos());
    if (anchor != lastAnchor)
    {
        emit linkHoverChanged(anchor);
        lastAnchor = anchor;
    }
    QGraphicsTextItem::hoverMoveEvent(event);
}

void ChatTextItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if (!lastAnchor.isEmpty())
        QDesktopServices::openUrl(lastAnchor);
    else
        emit clicked();
    QGraphicsTextItem::mousePressEvent(event);
}

ChatMessageWidget::ChatMessageWidget(bool received, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    show_date(true),
    show_sender(true),
    show_footer(true),
    maxWidth(2 * DATE_WIDTH)
{
    // set colors
    QColor baseColor = received ? QColor(0x26, 0x89, 0xd6) : QColor(0x7b, 0x7b, 0x7b);
    QColor backgroundColor = received ? QColor(0xe7, 0xf4, 0xfe) : QColor(0xfa, 0xfa, 0xfa);
    QColor shadowColor = QColor(0xd4, 0xd4, 0xd4);

    // draw header
    messageHeader = scene()->addPath( headerPath(MESSAGE_WIDTH), QPen(baseColor), QBrush(backgroundColor));
    messageHeader->setParentItem(this);
    messageHeader->setZValue(-1);

    // draw body
    messageBody = scene()->addPath( bodyPath(MESSAGE_WIDTH, BODY_HEIGHT), QPen(baseColor), QBrush(backgroundColor));
    messageBody->setParentItem(this);
    messageBody->setZValue(-1);

    // draw footer
    QLinearGradient shadowGradient(QPointF(0, 0), QPointF(0, 1));
    shadowGradient.setColorAt(0, shadowColor);
    shadowColor.setAlpha(0x00);
    shadowGradient.setColorAt(1, shadowColor);
    shadowGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    shadowGradient.setSpread(QGradient::PadSpread);
    messageFooter = scene()->addPath( footerPath(MESSAGE_WIDTH), QPen(Qt::white), QBrush(shadowGradient));
    messageFooter->setParentItem(this);
    messageFooter->setZValue(-1);
 
    // create text objects
    bodyText = new ChatTextItem;
    scene()->addItem(bodyText);
    bodyText->setParentItem(this);
    bodyText->setDefaultTextColor(Qt::black);

    dateText = scene()->addText("");
    QFont font = dateText->font();
    font.setPixelSize(10);
    dateText->setFont(font);
    dateText->setParentItem(this);
    dateText->setTextWidth(90);

    fromText = new ChatTextItem;
    scene()->addItem(fromText);
    fromText->setFont(font);
    fromText->setParentItem(this);
    fromText->setDefaultTextColor(baseColor);

    font.setPixelSize(12);
    bodyText->setFont(font);
    
    // set controls
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    connect(bodyText, SIGNAL(linkHoverChanged(QString)), this, SIGNAL(linkHoverChanged(QString)));
    connect(fromText, SIGNAL(clicked()), this, SLOT(slotTextClicked()));
}

/** Returns the QPainterPath used to draw the top of a chat bubble.
 */
QPainterPath ChatMessageWidget::headerPath(qreal width)
{
    QPainterPath path;
    path.moveTo(0, HEADER_HEIGHT);
    path.lineTo(0, 5);
    path.lineTo(20, 5);
    path.lineTo(17, 0);
    path.lineTo(27, 5);
    path.lineTo(width, 5);
    path.lineTo(width, HEADER_HEIGHT);
    return path;
}

/** Returns the QPainterPath used to draw the background of a chat bubble.
 */
QPainterPath ChatMessageWidget::bodyPath(qreal width, qreal height)
{
    QPainterPath path;
    path.moveTo(0,0);
    path.lineTo(0,height);
    path.lineTo(width,height);
    path.lineTo(width,0);
    return path;
}

/** Returns the QPainterPath used to draw the shadow under a chat bubble.
 */
QPainterPath ChatMessageWidget::footerPath(qreal width)
{
    QPainterPath path;
    path.moveTo(0,0);
    path.lineTo(0,FOOTER_HEIGHT);
    path.lineTo(width,FOOTER_HEIGHT);
    path.lineTo(width,0);
    return path;
}

bool ChatMessageWidget::collidesWithPath(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    return bodyText->collidesWithPath(path, mode);
}

ChatHistoryMessage ChatMessageWidget::message() const
{
    return msg;
}

QString ChatMessageWidget::selectedText() const
{
    return bodyText->textCursor().selectedText();
}

void ChatMessageWidget::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);

    // calculate space available for body
    qreal bodyHeight = rect.height();
    qreal bodyY = rect.y();
    if (show_sender)
    {
        bodyHeight -= (FROM_HEIGHT + HEADER_HEIGHT);
        bodyY += HEADER_HEIGHT + FROM_HEIGHT;
    }
    if (show_footer)
        bodyHeight -= (FOOTER_HEIGHT + 1);

    // position header
    if (show_sender)
    {
        fromText->setPos(rect.x(), rect.y());
        messageHeader->setPath(headerPath(rect.width()));
        messageHeader->setPos(rect.x(), rect.y() + FROM_HEIGHT);
    }

    // position body
    bodyText->setPos(rect.x() + BODY_OFFSET, bodyY - 3);
    messageBody->setPath(bodyPath(rect.width(), bodyHeight));
    messageBody->setPos(rect.x(), bodyY);

    // position the date
    if (show_date)
        dateText->setPos(rect.right() - (DATE_WIDTH + dateText->document()->idealWidth())/2, bodyText->y());

    // position the footer shadow
    if(show_footer)
    {
        messageFooter->setPath(footerPath(rect.width()));
        messageFooter->setPos(rect.x(), bodyY + bodyHeight + 1);
    }
}

void ChatMessageWidget::setMaximumWidth(qreal width)
{
    QGraphicsWidget::setMaximumWidth(width);
    maxWidth = width;
    bodyText->document()->setTextWidth(width - DATE_WIDTH - BODY_OFFSET);
    updateGeometry();
}

void ChatMessageWidget::setMessage(const ChatHistoryMessage &message)
{
    msg = message;

    /* set from */
    fromText->setPlainText(message.from);

    /* set date */
    QDateTime datetime = message.date.toLocalTime();
    dateText->setPlainText(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));

    /* set body */
    QString bodyHtml = Qt::escape(message.body);
    bodyHtml.replace("\n", "<br/>");
    QRegExp linkRegex("((ftp|http|https)://[^ ]+)");
    bodyHtml.replace(linkRegex, "<a href=\"\\1\">\\1</a>");
    bodyText->setHtml(bodyHtml);
}

void ChatMessageWidget::setSelection(const QRectF &rect)
{
    QRectF localRect = bodyText->mapRectFromScene(rect);
    localRect = bodyText->boundingRect().intersected(localRect);

    // determine selected text
    QAbstractTextDocumentLayout *layout = bodyText->document()->documentLayout();
    int startPos = layout->hitTest(localRect.topLeft(), Qt::FuzzyHit);
    int endPos = layout->hitTest(localRect.bottomRight(), Qt::FuzzyHit);

    // set cursor
    QTextCursor cursor(bodyText->document());
    cursor.setPosition(startPos);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    bodyText->setTextCursor(cursor);
}

/** Determine whether to show message sender and date depending
 *  on the previous message in the list.
 */
void ChatMessageWidget::setPrevious(ChatMessageWidget *previous)
{
    // calculate new values
    bool showSender = (!previous ||
        msg.fromJid != previous->message().fromJid ||
        msg.date > previous->message().date.addSecs(120 * 60));
    bool showDate = (showSender ||
        msg.date > previous->message().date.addSecs(60));
    bool showPreviousFooter = (previous && showSender);

    // check whether anything changed in sender or date
    if(showSender != show_sender || showDate != show_date)
    {
        // update values if changed
        setShowSender(showSender);
        setShowDate(showDate);
        updateGeometry();
    }
    // check whether anything changed in footer
    if(previous && previous->showFooter() != showPreviousFooter)
    {
        // update values if changed
        previous->setShowFooter(showPreviousFooter);
        previous->updateGeometry();
    }
}

void ChatMessageWidget::setShowDate(bool show)
{
    if (show)
    {
        dateText->show();
    } else {
        dateText->hide();
    }
    show_date = show;
}

bool ChatMessageWidget::showFooter()
{
    return show_footer;
}

void ChatMessageWidget::setShowFooter(bool show)
{
    if (show)
    {
        messageFooter->show();
    } else {
        messageFooter->hide();
    }
    show_footer = show;
}

void ChatMessageWidget::setShowSender(bool show)
{
    if (show)
    {
        messageHeader->show();
        fromText->show();
    } else {
        messageHeader->hide();
        fromText->hide();
    }
    show_sender = show;
}

QSizeF ChatMessageWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
            return bodyText->document()->size();
        case Qt::PreferredSize:
        {
            QSizeF hint(bodyText->document()->size());
            if (hint.width() < maxWidth)
                hint.setWidth(maxWidth);
            if (show_sender)
                hint.setHeight(hint.height() + HEADER_HEIGHT + FROM_HEIGHT);
            if (show_footer)
                hint.setHeight(hint.height() + FOOTER_HEIGHT);
            return hint;
        }
        default:
            return constraint;
    }
}

void ChatMessageWidget::slotTextClicked()
{
    emit messageClicked(msg);
}

ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene;
    setScene(scene);
    setDragMode(QGraphicsView::RubberBandDrag);
    setRubberBandSelectionMode(Qt::IntersectsItemShape);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setRenderHints(QPainter::Antialiasing);

    obj = new QGraphicsWidget;
    layout = new QGraphicsLinearLayout(Qt::Vertical);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(0);
    obj->setLayout(layout);
    scene->addItem(obj);

    connect(scene, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(copy()));

    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(selectAll()));
}

void ChatHistory::addMessage(const ChatHistoryMessage &message)
{
    if (message.body.isEmpty())
        return;

    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() > (scrollBar->maximum() - 10);

    /* position cursor */
    ChatMessageWidget *previous = NULL;
    int pos = 0;
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));

        // check for collision
        if (message.archived != child->message().archived &&
            message.fromJid == child->message().fromJid &&
            message.body == child->message().body &&
            qAbs(message.date.secsTo(child->message().date)) < 10)
            return;

        // we use greater or equal comparison (and not strictly greater) dates
        // because messages are usually received in chronological order
        if (message.date >= child->message().date)
        {
            previous = child;
            pos++;
        }
    }

    /* prepare message */
    ChatMessageWidget *msg = new ChatMessageWidget(message.received, obj);
    msg->setMessage(message);
    msg->setPrevious(previous);
    msg->setMaximumWidth(availableWidth());

    /* adjust next message */
    if (pos < layout->count())
    {
        ChatMessageWidget *next = static_cast<ChatMessageWidget*>(layout->itemAt(pos));
        next->setPrevious(msg);
    }

    /* insert new message */
    connect(msg, SIGNAL(linkHoverChanged(QString)), this, SLOT(slotLinkHoverChanged(QString)));
    connect(msg, SIGNAL(messageClicked(ChatHistoryMessage)), this, SIGNAL(messageClicked(ChatHistoryMessage)));
    layout->insertItem(pos, msg);
    adjustSize();

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
    qreal leftMargin = 0;
    qreal rightMargin = 0;
    layout->getContentsMargins(&leftMargin, 0, &rightMargin, 0);

    // FIXME : why do we need the extra 8 pixels?
    return width() - verticalScrollBar()->sizeHint().width() - leftMargin - rightMargin - 8;
}

void ChatHistory::clear()
{
    for (int i = layout->count() - 1; i >= 0; i--)
        delete layout->itemAt(i);
    adjustSize();
}

void ChatHistory::copy()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(copyText(), QClipboard::Clipboard);
}

/** Retrieve the text for the highlighted messages.
  */
QString ChatHistory::copyText()
{
    QString copyText;
    QList<QGraphicsItem*> selection = scene->selectedItems();

    // gather the message senders
    QSet<QString> senders;
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));
        if (selection.contains(child))
            senders.insert(child->message().from);
    }

    // copy selected messages
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));
        if (selection.contains(child))
        {
            ChatHistoryMessage message = child->message();

            if (!copyText.isEmpty())
                copyText += "\n";

            // if this is a conversation, prefix the message with its sender
            if (senders.size() > 1)
                copyText += message.from + "> ";

            copyText += child->selectedText().replace("\r\n", "\n");
        }
    }

#ifdef Q_OS_WIN
    return copyText.replace("\n", "\r\n");
#else
    return copyText;
#endif
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu;

    QAction *action = menu->addAction(tr("&Copy"));
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(copy()));

    action = menu->addAction(tr("Select &All"));
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(selectAll()));

    action = menu->addAction(QIcon(":/remove.png"), tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(clear()));

    menu->exec(event->globalPos());
    delete menu;
}

void ChatHistory::focusInEvent(QFocusEvent *e)
{
    QGraphicsView::focusInEvent(e);
    emit focused();
}

void ChatHistory::mouseMoveEvent(QMouseEvent *e)
{
    QGraphicsView::mouseMoveEvent(e);
    if (e->buttons() == Qt::LeftButton && !lastSelection.isEmpty())
    {
        QRectF rect = scene->selectionArea().boundingRect();
        foreach (ChatMessageWidget *child, lastSelection)
            child->setSelection(rect);
    }
}

void ChatHistory::mousePressEvent(QMouseEvent *e)
{
    // do not propagate right clicks, in order to preserve the selected text
    if (e->button() != Qt::RightButton)
        QGraphicsView::mousePressEvent(e);
}

void ChatHistory::resizeEvent(QResizeEvent *e)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() >= (scrollBar->maximum() - 10);

    const qreal w = availableWidth();
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));
        child->setMaximumWidth(w);
    }
    adjustSize();
    QGraphicsView::resizeEvent(e);

    /* scroll to end if we were previous at end */
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::selectAll()
{
    QPainterPath path;
    path.addRect(scene->sceneRect());
    scene->setSelectionArea(path);
}

void ChatHistory::slotLinkHoverChanged(const QString &link)
{
    setCursor(link.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
}

void ChatHistory::slotSelectionChanged()
{
    QRectF rect = scene->selectionArea().boundingRect();

    // highlight the selected items
    QList<QGraphicsItem*> selection = scene->selectedItems();
    QList<ChatMessageWidget*> newSelection;
    for (int i = 0; i < layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(layout->itemAt(i));
        if (selection.contains(child))
        {
            newSelection << child;
            child->setSelection(rect);
        } else if (lastSelection.contains(child)) {
            child->setSelection(QRectF());
        }
    }

    // for X11, copy the selected text to the selection buffer
    if (!selection.isEmpty())
    {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(copyText(), QClipboard::Selection);
    }

    lastSelection = newSelection;
}

ChatHistoryMessage::ChatHistoryMessage()
    : archived(false), received(true)
{
}
