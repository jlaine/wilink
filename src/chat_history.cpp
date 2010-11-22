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
#include <QDesktopServices>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QMenu>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStyle>
#include <QShortcut>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

#define BODY_OFFSET 5
#define BUBBLE_RADIUS 8
#define FOOTER_HEIGHT 5
#define MESSAGE_MAX 100

#ifdef WILINK_EMBEDDED
#define BODY_FONT 18
#define DATE_FONT 15
#define DATE_WIDTH 0
#define HEADER_HEIGHT 25
#else
#define BODY_FONT 12
#define DATE_FONT 10
#define DATE_WIDTH 80
#define HEADER_HEIGHT 20
#endif

/** Constructs a new ChatMessage.
 */
ChatMessage::ChatMessage()
    : archived(false), received(true)
{
}

/** Constructs a new ChatMesageBubble.
 *
 * @param parent
 */
ChatMessageBubble::ChatMessageBubble(bool received, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
     m_isReceived(received)
{
    QColor baseColor = received ? QColor(0x26, 0x89, 0xd6) : QColor(0x7b, 0x7b, 0x7b);
    QColor backgroundColor = received ? QColor(0xe7, 0xf4, 0xfe) : QColor(0xfa, 0xfa, 0xfa);
    QColor shadowColor = QColor(0xd4, 0xd4, 0xd4);

    // from
    m_from = new QGraphicsTextItem(this);
    QFont font = m_from->font();
    font.setPixelSize(DATE_FONT);
    m_from->setFont(font);
    m_from->setDefaultTextColor(baseColor);
    m_from->installSceneEventFilter(this);

    // bubble frame
    m_frame = new QGraphicsPathItem(this);
    m_frame->setPen(baseColor);
    m_frame->setBrush(backgroundColor);
    m_frame->setZValue(-1);

#if 0
    // bubble shadow
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(8);
    effect->setOffset(QPointF(2, 2));
    m_frame->setGraphicsEffect(effect);
#endif
}

/** Returns the index of a message within the bubble.
 *
 * @param widget
 */
int ChatMessageBubble::indexOf(ChatMessageWidget *widget) const
{
    return m_messages.indexOf(widget);
}

/** Inserts a message in the bubble at the given position.
 *
 * @param pos
 * @param widget
 */
void ChatMessageBubble::insertAt(int pos, ChatMessageWidget *widget)
{
    if (m_messages.isEmpty())
        m_from->setPlainText(widget->message().from);
    widget->setBubble(this);
    widget->setParentItem(this);
    widget->setMaximumWidth(m_maximumWidth - 2);
    connect(widget, SIGNAL(destroyed(QObject*)),
            this, SLOT(messageDestroyed(QObject*)));
    m_messages.insert(pos, widget);
    updateGeometry();
}

/** When a message is destroyed, update the bubble's internal list.
 *
 * @param obj
 */
void ChatMessageBubble::messageDestroyed(QObject *obj)
{
    m_messages.removeAll(static_cast<ChatMessageWidget*>(obj));
}

/** Filters events on the sender label.
 */
bool ChatMessageBubble::sceneEventFilter(QGraphicsItem *item, QEvent *event)
{
    if (item == m_from && !m_messages.isEmpty() &&
        event->type() == QEvent::GraphicsSceneMousePress)
    {
        emit messageClicked(m_messages[0]->message());
    }
    return false;
}

/** Sets the bubble's geometry.
 */
void ChatMessageBubble::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);
    rect.adjust(0.5, HEADER_HEIGHT + 0.5, -0.5, -FOOTER_HEIGHT - 0.5);

    QRectF arc(0, 0, 2 * BUBBLE_RADIUS, 2 * BUBBLE_RADIUS);

    // bubble top
    QPainterPath path;
    path.moveTo(rect.left(), rect.top() + BUBBLE_RADIUS);
    arc.moveTopLeft(rect.topLeft());
    path.arcTo(arc, 180, -90);
    path.lineTo(rect.left() + 20, rect.top());
    path.lineTo(rect.left() + 17, rect.top() - 5);
    path.lineTo(rect.left() + 27, rect.top());
    path.lineTo(rect.right() - BUBBLE_RADIUS, rect.top());
    arc.moveRight(rect.right());
    path.arcTo(arc, 90, -90);

    // bubble right, bottom, left
    path.lineTo(rect.right(), rect.bottom() - BUBBLE_RADIUS);
    arc.moveBottom(rect.bottom());
    path.arcTo(arc, 0, -90);
    path.lineTo(rect.left() + BUBBLE_RADIUS, rect.bottom());
    arc.moveLeft(rect.left());
    path.arcTo(arc, -90, -90);
    path.closeSubpath();
    m_frame->setPath(path);

    // messages
    rect.adjust(0.5, 0.5, -0.5, -0.5);
    const qreal x = rect.left();
    qreal y = rect.top();
    foreach (ChatMessageWidget *widget, m_messages)
    {
        const QRectF wRect(QPointF(x, y), widget->preferredSize());
        widget->setGeometry(wRect);
        y += wRect.height();
    }
}

/** Sets the bubble's maximum width.
 *
 * @param widget
 */
void ChatMessageBubble::setMaximumWidth(qreal width)
{
    m_maximumWidth = width;
    QGraphicsWidget::setMaximumWidth(width);
    foreach (ChatMessageWidget *child, m_messages)
        child->setMaximumWidth(m_maximumWidth - 2);
    updateGeometry();
}

/** Returns a size hint for the bubble.
 *
 * @param which
 * @param constraint
 */
QSizeF ChatMessageBubble::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
        {
            qreal height = HEADER_HEIGHT + FOOTER_HEIGHT + 2;
            foreach (ChatMessageWidget *widget, m_messages)
                height += widget->minimumHeight();
            return QSizeF(m_maximumWidth, height);
        }
        case Qt::PreferredSize:
        {
            qreal height = HEADER_HEIGHT + FOOTER_HEIGHT + 2;
            foreach (ChatMessageWidget *widget, m_messages)
                height += widget->preferredHeight();
            return QSizeF(m_maximumWidth, height);
        }
        default:
            return constraint;
    }
}

/** Moves all the messages after the given one to a new bubble.
 *
 * @param widget
 */
ChatMessageBubble *ChatMessageBubble::splitAfter(ChatMessageWidget *widget)
{
    const int index = m_messages.indexOf(widget);
    Q_ASSERT(index >= 0);

    // if this is the last message in the bubble, do nothing
    if (index == m_messages.size() - 1)
        return 0;

    ChatMessageBubble *bubble = new ChatMessageBubble(m_isReceived, parentItem());
    for (int i = m_messages.size() - 1; i > index; --i)
    {
        ChatMessageWidget *widget = m_messages[i];
        disconnect(widget, SIGNAL(destroyed(QObject*)),
                   this, SLOT(messageDestroyed(QObject*)));
        bubble->insertAt(0, widget);
        m_messages.removeAt(i);
    }
    updateGeometry();
    return bubble;
}

/** Constructs a new ChatMesageWidget.
 *
 * @param received
 * @param parent
 */
ChatMessageWidget::ChatMessageWidget(const ChatMessage &message, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    maxWidth(2 * DATE_WIDTH),
    m_bubble(0),
    m_message(message)
{
    const QColor textColor(Qt::black);

    // message body
    bodyText = new QGraphicsTextItem(this);
    QFont font = bodyText->font();
    font.setPixelSize(BODY_FONT);
    bodyText->setFont(font);
    bodyText->setDefaultTextColor(textColor);
    bodyText->setPos(BODY_OFFSET, 0);
    bodyText->installSceneEventFilter(this);

    QString bodyHtml = Qt::escape(message.body);
    bodyHtml.replace("\n", "<br/>");
    QRegExp linkRegex("((ftp|http|https)://[^ ]+)");
    bodyHtml.replace(linkRegex, "<a href=\"\\1\">\\1</a>");
    bodyText->setHtml(bodyHtml);

    // message date
#ifdef WILINK_EMBEDDED
    dateText = 0;
#else
    dateText = new QGraphicsTextItem(this);
    font = dateText->font();
    font.setPixelSize(DATE_FONT);
    dateText->setFont(font);
    dateText->setDefaultTextColor(textColor);
    dateText->setTextWidth(90);

    QDateTime datetime = message.date.toLocalTime();
    dateText->setPlainText(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
#endif

    // set controls
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

/** Returns the bubble this message belongs to.
 */
ChatMessageBubble *ChatMessageWidget::bubble()
{
    return m_bubble;
}

/** Break a text selection into a list of rectangles, which one rectangle per line.
 */
QList<RectCursor> ChatMessageWidget::chunkSelection(const QTextCursor &cursor) const
{
    QList<RectCursor> rectangles;

    const QTextLayout *layout = cursor.block().layout();
    const qreal margin = bodyText->document()->documentMargin();
    const int startPos = cursor.anchor() - cursor.block().position();
    const int endPos = cursor.position() - cursor.block().position();
    for (int i = 0; i < layout->lineCount(); ++i)
    {
        QTextLine line = layout->lineAt(i);
        const int lineEnd = line.textStart() + line.textLength();

        // skip lines before selection
        if (lineEnd <= startPos)
            continue;

        QRectF localRect = line.rect();
        QTextCursor localCursor = cursor;

        // position left edge
        if (line.textStart() < startPos)
        {
            QPointF topLeft = line.rect().topLeft();
            topLeft.setX(topLeft.x() + line.cursorToX(startPos));
            localRect.setTopLeft(topLeft);
            localCursor.setPosition(cursor.anchor());
        } else {
            localCursor.setPosition(line.textStart() + cursor.block().position());
        }

        // position right edge
        QPointF bottomRight = line.rect().bottomLeft();
        if (lineEnd > endPos)
        {
            bottomRight.setX(bottomRight.x() + line.cursorToX(endPos));
            localCursor.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        } else {
            bottomRight.setX(bottomRight.x() + line.cursorToX(lineEnd));
            localCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        }
        localRect.setBottomRight(bottomRight);

        // map to scene coordinates
        rectangles << qMakePair(bodyText->mapRectToScene(localRect.translated(margin, margin)), localCursor);

        if (lineEnd > endPos)
            break;
    }

    return rectangles;
}

/** Returns true if the message's body collides with the given path.
 *
 * @param path
 * @param mode
 */
bool ChatMessageWidget::collidesWithPath(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    return bodyText->collidesWithPath(path, mode);
}

/** Returns the message which this widget displays.
 */
ChatMessage ChatMessageWidget::message() const
{
    return m_message;
}

/** Filters events on the body label.
 */
bool ChatMessageWidget::sceneEventFilter(QGraphicsItem *item, QEvent *event)
{
    if (item == bodyText)
    {
        if (event->type() == QEvent::GraphicsSceneHoverLeave)
        {
            bodyAnchor = QString();
            bodyText->setCursor(Qt::ArrowCursor);
        }
        else if (event->type() == QEvent::GraphicsSceneHoverMove)
        {
            QGraphicsSceneHoverEvent *hoverEvent = static_cast<QGraphicsSceneHoverEvent*>(event);
            bodyAnchor = bodyText->document()->documentLayout()->anchorAt(hoverEvent->pos());
            bodyText->setCursor(bodyAnchor.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        }
        else if (event->type() == QEvent::GraphicsSceneMousePress)
        {
            QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (!bodyAnchor.isEmpty())
                    QDesktopServices::openUrl(bodyAnchor);
            }
        }
    }
    return false;
}

/** Sets the bubble this message belongs to.
 *
 * @param bubble
 */
void ChatMessageWidget::setBubble(ChatMessageBubble *bubble)
{
    m_bubble = bubble;
}

/** Sets the message's geometry.
 *
 * @param baseRect
 */
void ChatMessageWidget::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    // position the date
    if (dateText && dateText->isVisible())
    {
        QRectF rect(baseRect);
        rect.moveLeft(0);
        rect.moveTop(0);
        dateText->setPos(rect.right() - (DATE_WIDTH + dateText->document()->idealWidth())/2,
                         rect.top() + 2);
    }
}

/** Sets the message's maximum width.
 *
 * @param width
 */
void ChatMessageWidget::setMaximumWidth(qreal width)
{
    if (width == maxWidth)
        return;

    QGraphicsWidget::setMaximumWidth(width);
    maxWidth = width;
    bodyText->document()->setTextWidth(width - DATE_WIDTH - BODY_OFFSET);
    updateGeometry();
}

/** Sets the text cursor to select the given rectangle.
 *
 * @param rect
 */
void ChatMessageWidget::setSelection(const QRectF &rect)
{
    QRectF localRect = bodyText->boundingRect().intersected(bodyText->mapRectFromScene(rect));

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

/** Determines whether to show message date depending
 *  on the previous message in the list.
 */
void ChatMessageWidget::setPrevious(ChatMessageWidget *previous)
{
    if (!dateText)
        return;
    if (!previous ||
        m_message.fromJid != previous->message().fromJid ||
        m_message.date > previous->message().date.addSecs(60))
    {
        dateText->show();
    } else {
        dateText->hide();
    }
}

/** Returns the given size hint for the message.
 *
 * @param which
 * @param constraint
 */
QSizeF ChatMessageWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
        case Qt::PreferredSize:
        {
            QSizeF hint(bodyText->document()->size());
            if (hint.width() < maxWidth)
                hint.setWidth(maxWidth);
            return hint;
        }
        default:
            return constraint;
    }
}

/** Returns the message's text item.
 */
QGraphicsTextItem *ChatMessageWidget::textItem()
{
    return bodyText;
}

/** Constructs a new ChatHistoryWidget.
 *
 * @param parent
 */
ChatHistoryWidget::ChatHistoryWidget(QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_followEnd(true),
    m_view(0),
    m_lastFindWidget(0)
{
    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    m_trippleClickTimer = new QTimer(this);
    m_trippleClickTimer->setSingleShot(true);
    m_trippleClickTimer->setInterval(QApplication::doubleClickInterval());
}

void ChatHistoryWidget::addMessage(const ChatMessage &message)
{
    if (message.body.isEmpty())
        return;

    // check we hit the message limit and this message is too old
    if (m_messages.size() >= MESSAGE_MAX)
    {
        ChatMessageWidget *oldest = m_messages.first();
        if (message.date < oldest->message().date)
            return;
    }

    // position cursor
    int pos = 0;
    foreach (ChatMessageWidget *child, m_messages)
    {
        // check for collision
        if (message.archived != child->message().archived &&
            message.fromJid == child->message().fromJid &&
            message.body == child->message().body &&
            qAbs(message.date.secsTo(child->message().date)) < 10)
            return;

        // we use greater or equal comparison (and not strictly greater) dates
        // because messages are usually received in chronological order
        if (message.date >= child->message().date)
            pos++;
    }
    ChatMessageWidget *prevMsg = (pos > 0) ? m_messages[pos-1] : 0;
    ChatMessageWidget *nextMsg = (pos < m_messages.size()) ? m_messages[pos] : 0;

    // prepare message
    ChatMessageWidget *msg = new ChatMessageWidget(message, this);
    if (prevMsg)
        msg->setPrevious(prevMsg);
    if (nextMsg)
        nextMsg->setPrevious(msg);

    if (prevMsg && prevMsg->message().from == message.from)
    {
        // message belongs to the same bubble as previous message
        ChatMessageBubble *bubble = prevMsg->bubble();
        bubble->insertAt(bubble->indexOf(prevMsg) + 1, msg);
    }
    else if (nextMsg && nextMsg->message().from == message.from)
    {
        // message belongs to the same bubble as next message
        ChatMessageBubble *bubble = nextMsg->bubble();
        bubble->insertAt(bubble->indexOf(nextMsg), msg);
    }
    else
    {
        // split the previous bubble if needed
        int bubblePos = 0;
        if (prevMsg)
        {
            ChatMessageBubble *prevBubble = prevMsg->bubble();
            bubblePos = m_bubbles.indexOf(prevBubble) + 1;
            ChatMessageBubble *nextBubble = prevBubble->splitAfter(prevMsg);
            if (nextBubble)
                insertBubble(bubblePos, nextBubble);
        }

        // insert the new bubble
        ChatMessageBubble *bubble = new ChatMessageBubble(message.received, this);
        bubble->insertAt(0, msg);
        insertBubble(bubblePos, bubble);
    }

    connect(msg, SIGNAL(destroyed(QObject*)),
            this, SLOT(messageDestroyed(QObject*)));
    m_messages.insert(pos, msg);
    adjustSize();
}

/** Resizes the ChatHistoryWidget to its preferred size hint.
 */
void ChatHistoryWidget::adjustSize()
{
    QGraphicsWidget::adjustSize();

    // reposition search bubbles
    if (!m_glassItems.isEmpty() && m_lastFindWidget)
    {
        findClear();
        foreach (const RectCursor &textRect, m_lastFindWidget->chunkSelection(m_lastFindCursor))
        {
            ChatSearchBubble *glass = new ChatSearchBubble;
            glass->setSelection(textRect);
            scene()->addItem(glass);
            m_glassItems << glass;
        }
    }

    // adjust viewed rectangle
    if (m_messages.isEmpty())
        m_view->setSceneRect(0, 0, m_maximumWidth, 50);
    else
        m_view->setSceneRect(boundingRect());

    // scroll to end
    QScrollBar *scrollBar = m_view->verticalScrollBar();
    if (m_followEnd && scrollBar->value() < scrollBar->maximum())
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistoryWidget::bubbleDestroyed(QObject *obj)
{
    m_bubbles.removeAll(static_cast<ChatMessageBubble*>(obj));
}

/** Clears all messages.
 */
void ChatHistoryWidget::clear()
{
    QList<ChatMessageBubble*> bubbles = m_bubbles;
    m_bubbles.clear();
    m_messages.clear();
    m_selectedMessages.clear();
    for (int i = 0; i < bubbles.size(); ++i)
        delete bubbles[i];
    adjustSize();
}

/** Copies the selected text to the clipboard.
 */
void ChatHistoryWidget::copy()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(selectedText(), QClipboard::Clipboard);
}

/** When the viewport is resized, adjust the history widget's width.
 *
 * @param watched
 * @param event
 */
bool ChatHistoryWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize)
    {
        m_maximumWidth = m_view->viewport()->width() - 15;
        foreach (ChatMessageBubble *bubble, m_bubbles)
            bubble->setMaximumWidth(m_maximumWidth);
        adjustSize();
    }
    return false;
}

/** Find and highlight the given text.
 *
 * @param needle
 * @param flags
 * @param changed
 */
void ChatHistoryWidget::find(const QString &needle, QTextDocument::FindFlags flags, bool changed)
{
    // clear search bubbles
    findClear();

    // handle empty search
    if (needle.isEmpty())
    {
        emit findFinished(true);
        return;
    }

    // retrieve previous cursor
    QTextCursor cursor;
    int startIndex = (flags && QTextDocument::FindBackward) ? m_messages.size() -1 : 0;
    if (m_lastFindWidget)
    {
        for (int i = 0; i < m_messages.size(); ++i)
        {
            if (m_messages[i] == m_lastFindWidget)
            {
                startIndex = i;
                cursor = m_lastFindCursor;
                break;
            }
        }
    }
    if (changed)
        cursor.setPosition(cursor.anchor());

    // perform search
    bool looped = false;
    int i = startIndex;
    while (i >= 0 && i < m_messages.size())
    {
        ChatMessageWidget *child = m_messages[i];
        QTextDocument *document = child->textItem()->document();

        // position cursor
        if (cursor.isNull())
        {
            cursor = QTextCursor(document);
            if (flags && QTextDocument::FindBackward)
                cursor.movePosition(QTextCursor::End);
        }

        cursor = document->find(needle, cursor, flags);
        if (!cursor.isNull())
        {
            // build new glass
            QRectF boundingRect;
            foreach (const RectCursor &textRect, child->chunkSelection(cursor))
            {
                ChatSearchBubble *glass = new ChatSearchBubble;
                glass->setSelection(textRect);
                glass->bounce();
                scene()->addItem(glass);
                m_glassItems << glass;

                if (boundingRect.isEmpty())
                    boundingRect = glass->boundingRect();
                else
                    boundingRect = boundingRect.united(glass->boundingRect());
            }
            ensureVisible(boundingRect);

            m_lastFindCursor = cursor;
            m_lastFindWidget = child;
            emit findFinished(true);
            return;
        } else {
            if (looped)
                break;
            if (flags && QTextDocument::FindBackward) {
                if (--i < 0)
                    i = m_messages.size() - 1;
            } else {
                if (++i >= m_messages.size())
                    i = 0;
            }
            if (i == startIndex)
                looped = true;
        }
    }

    m_lastFindWidget = 0;
    m_lastFindCursor = QTextCursor();
    emit findFinished(false);
}

/** Clear all the search bubbles.
 */
void ChatHistoryWidget::findClear()
{
    foreach (ChatSearchBubble *item, m_glassItems)
    {
        scene()->removeItem(item);
        delete item;
    }
    m_glassItems.clear();
}

/** Inserts a bubble at the given position.
 *
 * @param pos
 * @param bubble
 */
void ChatHistoryWidget::insertBubble(int pos, ChatMessageBubble *bubble)
{
    bubble->setMaximumWidth(m_maximumWidth);
    m_bubbles.insert(pos, bubble);
    m_layout->insertItem(pos, bubble);

    bool check;
    check = connect(bubble, SIGNAL(destroyed(QObject*)),
                    this, SLOT(bubbleDestroyed(QObject*)));
    Q_ASSERT(check);

    check = connect(bubble, SIGNAL(messageClicked(ChatMessage)),
                    this, SIGNAL(messageClicked(ChatMessage)));
    Q_ASSERT(check);
}

void ChatHistoryWidget::messageDestroyed(QObject *obj)
{
    ChatMessageWidget *widget = static_cast<ChatMessageWidget*>(obj);
    m_messages.removeAll(widget);
    m_selectedMessages.removeAll(widget);
}

ChatMessageWidget *ChatHistoryWidget::messageWidgetAt(const QPointF &pos) const
{
    QGraphicsItem *hit = scene()->itemAt(pos);
    while (hit)
    {
        ChatMessageWidget *widget = static_cast<ChatMessageWidget*>(hit);
        if (m_messages.contains(widget))
            return widget;
        hit = hit->parentItem();
    }
    return 0;
}

void ChatHistoryWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    ChatMessageWidget *selected = messageWidgetAt(event->pos());
    if (event->buttons() != Qt::LeftButton || !selected)
        return;

    QGraphicsTextItem *textItem = selected->textItem();
    QAbstractTextDocumentLayout *layout = textItem->document()->documentLayout();
    int cursorPos = layout->hitTest(textItem->mapFromScene(event->pos()), Qt::FuzzyHit);
    if (cursorPos != -1)
    {
        QTextCursor cursor = textItem->textCursor();
        cursor.setPosition(cursorPos);
        cursor.select(QTextCursor::WordUnderCursor);
        textItem->setTextCursor(cursor);

        // update selection
        foreach (ChatMessageWidget *child, m_selectedMessages)
            if (child != selected)
                child->setSelection(QRectF());
        m_selectedMessages.clear();
        m_selectedMessages << selected;
        QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);

        m_trippleClickTimer->start();
        event->accept();
    }
}

void ChatHistoryWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->buttons() == Qt::LeftButton)
    {
        QPainterPath path;
        path.addRect(QRectF(m_selectionStart, e->pos()));
        scene()->setSelectionArea(path, Qt::IntersectsItemBoundingRect);
        slotSelectionChanged();
        e->accept();
    }
}

void ChatHistoryWidget::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->buttons() != Qt::LeftButton)
        return;

    // clear selection
    m_selectionStart = e->pos();
    scene()->setSelectionArea(QPainterPath());

    // handle tripple click
    ChatMessageWidget *selected = messageWidgetAt(e->pos());
    if (selected && m_trippleClickTimer->isActive())
    {
        QTextCursor cursor = selected->textItem()->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        selected->textItem()->setTextCursor(cursor);

        // update selection
        foreach (ChatMessageWidget *child, m_selectedMessages)
            if (child != selected)
                child->setSelection(QRectF());
        m_selectedMessages.clear();
        m_selectedMessages << selected;
        QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);
    }
    e->accept();
}

/** Selects all the messages.
 */
void ChatHistoryWidget::selectAll()
{
    QPainterPath path;
    path.addRect(scene()->sceneRect());
    scene()->setSelectionArea(path);
}

/** Retrieves the selected text.
 */
QString ChatHistoryWidget::selectedText() const
{
    QString copyText;

    // gather the message senders
    QSet<QString> senders;
    foreach (ChatMessageWidget *child, m_selectedMessages)
        senders.insert(child->message().from);

    // copy selected messages
    foreach (ChatMessageWidget *child, m_selectedMessages)
    {
        ChatMessage message = child->message();

        if (!copyText.isEmpty())
            copyText += "\n";

        // if this is a conversation, prefix the message with its sender
        if (senders.size() > 1)
            copyText += message.from + "> ";

        copyText += child->textItem()->textCursor().selectedText().replace("\r\n", "\n");
    }

#ifdef Q_OS_WIN
    return copyText.replace("\n", "\r\n");
#else
    return copyText;
#endif
}

void ChatHistoryWidget::setView(QGraphicsView *view)
{
    bool check;
    m_view = view;

    /* set up hooks */
    m_view->viewport()->installEventFilter(this);

    check = connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                    this, SLOT(slotScrollChanged()));
    Q_ASSERT(check);

    check = connect(view->scene(), SIGNAL(selectionChanged()),
                    this, SLOT(slotSelectionChanged()));
    Q_ASSERT(check);

    /* set up keyboard shortcuts */
    QAction *action = new QAction(tr("&Copy"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(copy()));
    Q_ASSERT(check);
    m_view->addAction(action);

    action = new QAction(tr("Select &All"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(selectAll()));
    Q_ASSERT(check);
    m_view->addAction(action);

    action = new QAction(tr("Clear"), this);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_X));
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(clear()));
    Q_ASSERT(check);
    m_view->addAction(action);

    // initialise size
    adjustSize();
}

/** When the scroll value changes, remember whether we were at the end
 *  of the view.
 */
void ChatHistoryWidget::slotScrollChanged()
{
    QScrollBar *scrollBar = m_view->verticalScrollBar();
    m_followEnd = scrollBar->value() >= scrollBar->maximum() - 16;
}

/** Handles a rubberband selection.
 */
void ChatHistoryWidget::slotSelectionChanged()
{
    const QRectF rect = scene()->selectionArea().boundingRect();
    const QList<QGraphicsItem*> selection = scene()->selectedItems();

    // update the selected items
    QList<ChatMessageWidget*> newSelection;
    foreach (ChatMessageWidget *child, m_messages)
    {
        if (selection.contains(child))
        {
            newSelection << child;
            child->setSelection(rect);
        } else if (m_selectedMessages.contains(child)) {
            child->setSelection(QRectF());
        }
    }
    m_selectedMessages = newSelection;

    // for X11, copy the selected text to the selection buffer
    if (!m_selectedMessages.isEmpty())
        QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);
}

ChatSearchBubble::ChatSearchBubble()
    : m_margin(3), m_radius(4)
{
    // shadow
    shadow = new QGraphicsPathItem;
    QColor shadowColor(0x99, 0x99, 0x99);
    QLinearGradient shadowGradient(QPointF(0, 0), QPointF(0, 1));
    shadowGradient.setColorAt(0, shadowColor);
    shadowColor.setAlpha(0xff);
    shadowGradient.setColorAt(.95, shadowColor);
    shadowColor.setAlpha(0x0);
    shadowGradient.setColorAt(1, shadowColor);
    shadowGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    shadowGradient.setSpread(QGradient::PadSpread);
    shadow->setPen(Qt::NoPen);
    shadow->setBrush(shadowGradient);
    addToGroup(shadow);

    // bubble
    bubble = new QGraphicsPathItem;
    bubble->setPen(QColor(63, 63, 63, 95));
    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0, QColor(255, 255, 0, 255));
    gradient.setColorAt(1, QColor(228, 212, 0, 255));
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setSpread(QGradient::PadSpread);
    bubble->setBrush(gradient);
    addToGroup(bubble);

    // text
    text = new QGraphicsTextItem;
    QFont font = text->font();
    font.setPixelSize(BODY_FONT);
    text->setFont(font);
    addToGroup(text);

    setZValue(1);
}

void ChatSearchBubble::bounce()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "scale");
    animation->setDuration(250);
    animation->setStartValue(1);
    animation->setKeyValueAt(0.5, 1.5);
    animation->setEndValue(1);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QRectF ChatSearchBubble::boundingRect() const
{
    return bubble->boundingRect();
}

void ChatSearchBubble::setSelection(const RectCursor &selection)
{
    m_selection = selection;

    setTransformOriginPoint(m_selection.first.center());

    QPointF textPos = m_selection.first.topLeft();
    textPos.setX(textPos.x() - text->document()->documentMargin());
    textPos.setY(textPos.y() - text->document()->documentMargin());
    text->setPos(textPos);
    text->setPlainText(m_selection.second.selectedText());

    QRectF glassRect;
    glassRect.setX(m_selection.first.x() - m_margin);
    glassRect.setY(m_selection.first.y() - m_margin);
    glassRect.setWidth(m_selection.first.width() + 2 * m_margin);
    glassRect.setHeight(m_selection.first.height() + 2 * m_margin);

    QPainterPath path;
    path.addRoundedRect(glassRect, m_radius, m_radius);
    bubble->setPath(path);

    QPainterPath shadowPath;
    glassRect.moveTop(glassRect.top() + 2);
    shadowPath.addRoundedRect(glassRect, m_radius, m_radius);
    shadow->setPath(shadowPath);
}
