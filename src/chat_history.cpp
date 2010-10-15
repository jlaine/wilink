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
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

#ifdef WILINK_EMBEDDED
#include "flickcharm.h"
#endif

#define DATE_WIDTH 80
#define FROM_HEIGHT 15
#define HEADER_HEIGHT 10
#define BODY_OFFSET 5
#define FOOTER_HEIGHT 5
#define MESSAGE_MAX 100

#ifdef WILINK_EMBEDDED
#define BODY_FONT 14
#define DATE_FONT 12
#else
#define BODY_FONT 12
#define DATE_FONT 10
#endif

/** Constructs a new ChatMessage.
 */
ChatMessage::ChatMessage()
    : archived(false), received(true)
{
}

/** Constructs a new ChatMesageWidget.
 *
 * @param received
 * @param parent
 */
ChatMessageWidget::ChatMessageWidget(bool received, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    maxWidth(2 * DATE_WIDTH),
    show_date(true),
    show_footer(true),
    show_sender(true)
{
    // set colors
    QColor baseColor = received ? QColor(0x26, 0x89, 0xd6) : QColor(0x7b, 0x7b, 0x7b);
    QColor backgroundColor = received ? QColor(0xe7, 0xf4, 0xfe) : QColor(0xfa, 0xfa, 0xfa);
    QColor shadowColor = QColor(0xd4, 0xd4, 0xd4);

    // draw body
    messageBackground = scene()->addPath(QPainterPath(), QPen(Qt::NoPen), QBrush(backgroundColor));
    messageBackground->setParentItem(this);
    messageBackground->setZValue(-1);
    messageFrame = scene()->addPath(QPainterPath(), QPen(baseColor), QBrush(Qt::NoBrush));
    messageFrame->setParentItem(this);

    // draw shadow
    QLinearGradient shadowGradient(QPointF(0, 0), QPointF(0, 1));
    shadowGradient.setColorAt(0, shadowColor);
    shadowColor.setAlpha(0x00);
    shadowGradient.setColorAt(1, shadowColor);
    shadowGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    shadowGradient.setSpread(QGradient::PadSpread);
    messageShadow = scene()->addRect(QRectF(), QPen(Qt::white), QBrush(shadowGradient));
    messageShadow->setParentItem(this);
    messageShadow->setZValue(-2);
 
    // create text objects
    bodyText = new QGraphicsTextItem(this);
    bodyText->setDefaultTextColor(Qt::black);
    QFont font = bodyText->font();
    font.setPixelSize(BODY_FONT);
    bodyText->setFont(font);
    bodyText->installSceneEventFilter(this);

    dateText = new QGraphicsTextItem(this);
    font = dateText->font();
    font.setPixelSize(DATE_FONT);
    dateText->setFont(font);
    dateText->setTextWidth(90);

    fromText = new QGraphicsTextItem(this);
    fromText->setFont(font);
    fromText->setDefaultTextColor(baseColor);
    fromText->installSceneEventFilter(this);

    // set controls
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

/** Returns the QPainterPath used to draw the chat bubble.
 */
QPainterPath ChatMessageWidget::bodyPath(qreal width, qreal height, bool close)
{
    QPainterPath path;
    qreal bodyY = 0;

    // top
    if (show_sender)
    {
        path.moveTo(0, HEADER_HEIGHT);
        path.lineTo(0, 5);
        path.lineTo(20, 5);
        path.lineTo(17, 0);
        path.lineTo(27, 5);
        path.lineTo(width, 5);
        path.lineTo(width, HEADER_HEIGHT);
        bodyY += HEADER_HEIGHT;
    } else {
        path.moveTo(0, 0);
        if (close)
            path.lineTo(width, bodyY);
        else
            path.moveTo(width, bodyY);
    }

    // right
    path.lineTo(width, bodyY + height);

    // bottom
    if (close || show_footer)
        path.lineTo(0, bodyY + height);
    else
        path.moveTo(0, bodyY + height);

    // left
    path.lineTo(0, bodyY);
    return path;
}

bool ChatMessageWidget::collidesWithPath(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    return bodyText->collidesWithPath(path, mode);
}

/** Filters events on the "from" and "body" labels.
 */
bool ChatMessageWidget::sceneEventFilter(QGraphicsItem *item, QEvent *event)
{
    if (item == fromText && event->type() == QEvent::GraphicsSceneMousePress)
    {
        emit messageClicked(msg);
    }
    else if (item == bodyText)
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

void ChatMessageWidget::setGeometry(const QRectF &baseRect)
{
    QGraphicsWidget::setGeometry(baseRect);

    QRectF rect(baseRect);
    rect.moveLeft(0);
    rect.moveTop(0);

    // calculate space available for body
    qreal bodyHeight = rect.height();
    qreal frameTop = rect.y();
    qreal textY = rect.y() - 1;
    if (show_sender)
    {
        bodyHeight -= (FROM_HEIGHT + HEADER_HEIGHT);
        frameTop += FROM_HEIGHT;
        textY += HEADER_HEIGHT + FROM_HEIGHT - 2;
    }
    if (show_footer)
        bodyHeight -= FOOTER_HEIGHT;

    // position sender
    if (show_sender)
        fromText->setPos(rect.x(), rect.y());

    // position body
    bodyText->setPos(rect.x() + BODY_OFFSET, textY);
    messageBackground->setPath(bodyPath(rect.width(), bodyHeight, true));
    messageBackground->setPos(rect.x(), frameTop);
    messageFrame->setPath(bodyPath(rect.width(), bodyHeight, false));
    messageFrame->setPos(rect.x(), frameTop);

    // position the date
    if (show_date)
        dateText->setPos(rect.right() - (DATE_WIDTH + dateText->document()->idealWidth())/2, bodyText->y());

    // position the footer shadow
    if(show_footer)
    {
        messageShadow->setRect(0, 0, rect.width(), FOOTER_HEIGHT);
        messageShadow->setPos(rect.x(), rect.height() - FOOTER_HEIGHT);
    }
}

void ChatMessageWidget::setMaximumWidth(qreal width)
{
    if (width == maxWidth)
        return;

    // FIXME : do we actually need to pass the width to the base class?
    QGraphicsWidget::setMaximumWidth(width);
    maxWidth = width;
    bodyText->document()->setTextWidth(width - DATE_WIDTH - BODY_OFFSET);
    updateGeometry();
}

/** Returns the message which this widget displays.
 */
ChatMessage ChatMessageWidget::message() const
{
    return msg;
}

/** Sets the message which this widget displays.
 */
void ChatMessageWidget::setMessage(const ChatMessage &message)
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

/** Set the text cursor to select the given rectangle.
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
    if(previous && previous->show_footer != showPreviousFooter)
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

void ChatMessageWidget::setShowFooter(bool show)
{
    if (show)
    {
        messageShadow->show();
    } else {
        messageShadow->hide();
    }
    show_footer = show;
}

void ChatMessageWidget::setShowSender(bool show)
{
    if (show)
    {
        fromText->show();
    } else {
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

QTextDocument *ChatMessageWidget::document() const
{
    return bodyText->document();
}

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
    m_lastFindWidget(0)
{
    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_layout->setContentsMargins(5, 0, 5, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);

    m_trippleClickTimer = new QTimer(this);
    m_trippleClickTimer->setSingleShot(true);
    m_trippleClickTimer->setInterval(QApplication::doubleClickInterval());

    bool check;
    check = connect(this, SIGNAL(geometryChanged()),
                    this, SLOT(slotGeometryChanged()));
    Q_ASSERT(check);
    Q_UNUSED(check);
}

ChatMessageWidget *ChatHistoryWidget::addMessage(const ChatMessage &message)
{
    if (message.body.isEmpty())
        return 0;

    // check we hit the message limit and this message is too old
    if (m_layout->count() >= MESSAGE_MAX)
    {
        ChatMessageWidget *oldest = static_cast<ChatMessageWidget*>(m_layout->itemAt(0));
        if (message.date < oldest->message().date)
            return 0;
    }

    /* position cursor */
    ChatMessageWidget *previous = NULL;
    int pos = 0;
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));

        // check for collision
        if (message.archived != child->message().archived &&
            message.fromJid == child->message().fromJid &&
            message.body == child->message().body &&
            qAbs(message.date.secsTo(child->message().date)) < 10)
            return 0;

        // we use greater or equal comparison (and not strictly greater) dates
        // because messages are usually received in chronological order
        if (message.date >= child->message().date)
        {
            previous = child;
            pos++;
        }
    }

    /* prepare message */
    ChatMessageWidget *msg = new ChatMessageWidget(message.received, this);
    msg->setMessage(message);
    msg->setPrevious(previous);
    msg->setMaximumWidth(m_maximumWidth);

    /* adjust next message */
    if (pos < m_layout->count())
    {
        ChatMessageWidget *next = static_cast<ChatMessageWidget*>(m_layout->itemAt(pos));
        next->setPrevious(msg);
    }

    /* insert new message */
    bool check;
    check = connect(msg, SIGNAL(messageClicked(ChatMessage)),
                    this, SIGNAL(messageClicked(ChatMessage)));
    Q_ASSERT(check);

    m_layout->insertItem(pos, msg);
    adjustSize();

    return msg;
}

/** Clears all messages.
 */
void ChatHistoryWidget::clear()
{
    m_selectedMessages.clear();
    for (int i = m_layout->count() - 1; i >= 0; i--)
        delete m_layout->itemAt(i);
    adjustSize();
}

/** Copies the selected text to the clipboard.
 */
void ChatHistoryWidget::copy()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(selectedText(), QClipboard::Clipboard);
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
    int startIndex = (flags && QTextDocument::FindBackward) ? m_layout->count() -1 : 0;
    if (m_lastFindWidget)
    {
        for (int i = 0; i < m_layout->count(); ++i)
        {
            if (m_layout->itemAt(i) == m_lastFindWidget)
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
    while (i >= 0 && i < m_layout->count())
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));

        // position cursor
        if (cursor.isNull())
        {
            cursor = QTextCursor(child->document());
            if (flags && QTextDocument::FindBackward)
                cursor.movePosition(QTextCursor::End);
        }

        cursor = child->document()->find(needle, cursor, flags);
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
                    i = m_layout->count() - 1;
            } else {
                if (++i >= m_layout->count())
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

ChatMessageWidget *ChatHistoryWidget::messageWidgetAt(const QPointF &pos) const
{
    QGraphicsItem *hit = scene()->itemAt(pos);
    while (hit)
    {
        if (hit->parentItem() == this)
            return static_cast<ChatMessageWidget*>(hit);
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
        scene()->setSelectionArea(path);
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

/** Sets the maximum width for the history.
 *
 * @param width
 */
void ChatHistoryWidget::setMaximumWidth(qreal width)
{
    if (width == m_maximumWidth)
        return;

    m_maximumWidth = width;
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
        child->setMaximumWidth(width);
    }
    adjustSize();
}

/** When the history geometry changes, reposition search bubbles.
 */
void ChatHistoryWidget::slotGeometryChanged()
{
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
}

/** Handles a rubberband selection.
 */
void ChatHistoryWidget::slotSelectionChanged()
{
    const QRectF rect = scene()->selectionArea().boundingRect();
    const QList<QGraphicsItem*> selection = scene()->selectedItems();

    // update the selected items
    QList<ChatMessageWidget*> newSelection;
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
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

ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent),
    m_followEnd(true)
{
    bool check;
    QGraphicsScene *scene = new QGraphicsScene(this);
    setScene(scene);
#ifdef WILINK_EMBEDDED
    FlickCharm *charm = new FlickCharm(this);
    charm->activateOn(this);
#else
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif

    m_obj = new ChatHistoryWidget;
    scene->addItem(m_obj);

    check = connect(m_obj, SIGNAL(geometryChanged()),
                    this, SLOT(historyChanged()));
    Q_ASSERT(check);

    check = connect(scene, SIGNAL(selectionChanged()),
                    m_obj, SLOT(slotSelectionChanged()));
    Q_ASSERT(check);

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C), this);
    check = connect(shortcut, SIGNAL(activated()),
                    m_obj, SLOT(copy()));
    Q_ASSERT(check);

    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A), this);
    check = connect(shortcut, SIGNAL(activated()),
                    m_obj, SLOT(selectAll()));
    Q_ASSERT(check);
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu;

    QAction *action = menu->addAction(tr("&Copy"));
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C));
    connect(action, SIGNAL(triggered(bool)), m_obj, SLOT(copy()));

    action = menu->addAction(tr("Select &All"));
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A));
    connect(action, SIGNAL(triggered(bool)), m_obj, SLOT(selectAll()));

    action = menu->addAction(QIcon(":/remove.png"), tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), m_obj, SLOT(clear()));

    menu->exec(event->globalPos());
    delete menu;
}

/** When the ChatHistoryWidget changes geometry, adjust the
 *  view's scene rectangle.
 */
void ChatHistory::historyChanged()
{
    QRectF rect = m_obj->boundingRect();
    rect.setHeight(rect.height() - 10);
    setSceneRect(rect);
    if (m_followEnd)
    {
        QScrollBar *scrollBar = verticalScrollBar();
        scrollBar->setSliderPosition(scrollBar->maximum());
    }
}

ChatHistoryWidget *ChatHistory::historyWidget()
{
    return m_obj;
}

void ChatHistory::resizeEvent(QResizeEvent *e)
{
    // calculate available width
    qreal leftMargin = 0;
    qreal rightMargin = 0;
    m_obj->layout()->getContentsMargins(&leftMargin, 0, &rightMargin, 0);
    // FIXME : why do we need the extra 8 pixels?
    const qreal w = width() - verticalScrollBar()->sizeHint().width() - leftMargin - rightMargin - 8;

    // update history widget
    m_obj->setMaximumWidth(w);
    historyChanged();

    QGraphicsView::resizeEvent(e);
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
    animation->start();
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
