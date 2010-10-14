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

enum MessageRole
{
    ArchivedRole,
    BodyRole,
    DateRole,
    FromRole,
    FromJidRole,
    ReceivedRole,
};

ChatHistoryModel::ChatHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roleNames;
    roleNames[ArchivedRole] = "archived";
    roleNames[BodyRole] = "body";
    roleNames[DateRole] = "date";
    roleNames[FromRole] = "from";
    roleNames[FromJidRole] = "fromJid";
    roleNames[ReceivedRole] = "received";
    setRoleNames(roleNames);
}

void ChatHistoryModel::addMessage(const ChatHistoryMessage &message)
{
    int pos = 0;
    foreach (const ChatHistoryMessage &current, m_messages)
    {
        if (message.date >= current.date)
            pos++;
        else
            break;
    }
    beginInsertRows(QModelIndex(), pos, pos);
    m_messages.insert(pos, message);
    endInsertRows();
}

QVariant ChatHistoryModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_messages.size())
        return QVariant();

    switch (role)
    {
    case ArchivedRole:
        return m_messages[row].archived;
    case BodyRole:
        return m_messages[row].body;
    case DateRole:
        return m_messages[row].date;
    case FromRole:
        return m_messages[row].from;
    case FromJidRole:
        return m_messages[row].fromJid;
    case ReceivedRole:
        return m_messages[row].received;
    default:
        return QVariant();
    }
}

int ChatHistoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_messages.size();
}

void ChatTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!lastAnchor.isEmpty())
    {
        setCursor(Qt::ArrowCursor);
        lastAnchor = "";
    }
    QGraphicsTextItem::hoverMoveEvent(event);
}

void ChatTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QString anchor = document()->documentLayout()->anchorAt(event->pos());
    if (anchor != lastAnchor)
    {
        setCursor(anchor.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
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
    bodyText = new ChatTextItem;
    scene()->addItem(bodyText);
    bodyText->setParentItem(this);
    bodyText->setDefaultTextColor(Qt::black);
    QFont font = bodyText->font();
    font.setPixelSize(BODY_FONT);
    bodyText->setFont(font);

    dateText = scene()->addText("");
    font = dateText->font();
    font.setPixelSize(DATE_FONT);
    dateText->setFont(font);
    dateText->setParentItem(this);
    dateText->setTextWidth(90);

    fromText = new ChatTextItem;
    scene()->addItem(fromText);
    fromText->setFont(font);
    fromText->setParentItem(this);
    fromText->setDefaultTextColor(baseColor);

    // set controls
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    connect(fromText, SIGNAL(clicked()), this, SLOT(slotTextClicked()));
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

ChatHistoryMessage ChatMessageWidget::message() const
{
    return msg;
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

void ChatMessageWidget::slotTextClicked()
{
    emit messageClicked(msg);
}

QTextDocument *ChatMessageWidget::document() const
{
    return bodyText->document();
}

QTextCursor ChatMessageWidget::textCursor() const
{
    return bodyText->textCursor();
}

void ChatMessageWidget::setTextCursor(const QTextCursor &cursor)
{
    bodyText->setTextCursor(cursor);
}

ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent),
    m_lastFindWidget(0)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setDragMode(QGraphicsView::RubberBandDrag);
    setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);
#ifdef WILINK_EMBEDDED
    FlickCharm *charm = new FlickCharm(this);
    charm->activateOn(this);
#else
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif

    m_obj = new QGraphicsWidget;
    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_layout->setContentsMargins(5, 0, 5, 0);
    m_layout->setSpacing(0);
    m_obj->setLayout(m_layout);
    m_scene->addItem(m_obj);

    connect(m_scene, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));

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

    // check we hit the message limit and this message is too old
    if (m_layout->count() >= MESSAGE_MAX)
    {
        ChatMessageWidget *oldest = static_cast<ChatMessageWidget*>(m_layout->itemAt(0));
        if (message.date < oldest->message().date)
            return;
    }

    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() > (scrollBar->maximum() - 10);

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
    ChatMessageWidget *msg = new ChatMessageWidget(message.received, m_obj);
    msg->setMessage(message);
    msg->setPrevious(previous);
    msg->setMaximumWidth(availableWidth());

    /* adjust next message */
    if (pos < m_layout->count())
    {
        ChatMessageWidget *next = static_cast<ChatMessageWidget*>(m_layout->itemAt(pos));
        next->setPrevious(msg);
    }

    /* insert new message */
    connect(msg, SIGNAL(linkHoverChanged(QString)), this, SLOT(slotLinkHoverChanged(QString)));
    connect(msg, SIGNAL(messageClicked(ChatHistoryMessage)), this, SIGNAL(messageClicked(ChatHistoryMessage)));
    m_layout->insertItem(pos, msg);
    adjustSize();

    /* scroll to end if we were previous at end */
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::adjustSize()
{
    m_obj->adjustSize();
    QRectF rect = m_obj->boundingRect();
    rect.setHeight(rect.height() - 10);
    setSceneRect(rect);
}

qreal ChatHistory::availableWidth() const
{
    qreal leftMargin = 0;
    qreal rightMargin = 0;
    m_layout->getContentsMargins(&leftMargin, 0, &rightMargin, 0);

    // FIXME : why do we need the extra 8 pixels?
    return width() - verticalScrollBar()->sizeHint().width() - leftMargin - rightMargin - 8;
}

void ChatHistory::clear()
{
    for (int i = m_layout->count() - 1; i >= 0; i--)
        delete m_layout->itemAt(i);
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
    QList<QGraphicsItem*> selection = m_scene->selectedItems();

    // gather the message senders
    QSet<QString> senders;
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
        if (selection.contains(child))
            senders.insert(child->message().from);
    }

    // copy selected messages
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
        if (selection.contains(child))
        {
            ChatHistoryMessage message = child->message();

            if (!copyText.isEmpty())
                copyText += "\n";

            // if this is a conversation, prefix the message with its sender
            if (senders.size() > 1)
                copyText += message.from + "> ";

            copyText += child->textCursor().selectedText().replace("\r\n", "\n");
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

/** Find and highlight the given text.
 *
 * @param needle
 * @param flags
 * @param changed
 */
void ChatHistory::find(const QString &needle, QTextDocument::FindFlags flags, bool changed)
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
                m_scene->addItem(glass);
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
void ChatHistory::findClear()
{
    foreach (ChatSearchBubble *item, m_glassItems)
    {
        m_scene->removeItem(item);
        delete item;
    }
    m_glassItems.clear();
}

void ChatHistory::focusInEvent(QFocusEvent *e)
{
    QGraphicsView::focusInEvent(e);
    emit focused();
}

void ChatHistory::mouseMoveEvent(QMouseEvent *e)
{
    QGraphicsView::mouseMoveEvent(e);
    if (e->buttons() == Qt::LeftButton && !m_lastSelection.isEmpty())
    {
        QRectF rect = m_scene->selectionArea().boundingRect();
        foreach (ChatMessageWidget *child, m_lastSelection)
            child->setSelection(rect);
    }
}

void ChatHistory::mousePressEvent(QMouseEvent *e)
{
    // clear search bubbles
    findClear();

    // do not propagate right clicks, in order to preserve the selected text
    if (e->button() != Qt::RightButton)
        QGraphicsView::mousePressEvent(e);
}

void ChatHistory::resizeEvent(QResizeEvent *e)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() >= (scrollBar->maximum() - 10);

    // resize widgets
    const qreal w = availableWidth();
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
        child->setMaximumWidth(w);
    }
    adjustSize();
    QGraphicsView::resizeEvent(e);

    // reposition search bubbles
    const bool hadBubbles = m_glassItems.size() > 0;
    findClear();
    if (hadBubbles && m_lastFindWidget)
    {
        foreach (const RectCursor &textRect, m_lastFindWidget->chunkSelection(m_lastFindCursor))
        {
            ChatSearchBubble *glass = new ChatSearchBubble;
            glass->setSelection(textRect);
            m_scene->addItem(glass);
            m_glassItems << glass;
        }
    }

    // scroll to end if we were previous at end
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::selectAll()
{
    QPainterPath path;
    path.addRect(m_scene->sceneRect());
    m_scene->setSelectionArea(path);
}

void ChatHistory::slotSelectionChanged()
{
    QRectF rect = m_scene->selectionArea().boundingRect();

    // highlight the selected items
    QList<QGraphicsItem*> selection = m_scene->selectedItems();
    QList<ChatMessageWidget*> newSelection;
    for (int i = 0; i < m_layout->count(); i++)
    {
        ChatMessageWidget *child = static_cast<ChatMessageWidget*>(m_layout->itemAt(i));
        if (selection.contains(child))
        {
            newSelection << child;
            child->setSelection(rect);
        } else if (m_lastSelection.contains(child)) {
            child->setSelection(QRectF());
        }
    }

    // for X11, copy the selected text to the selection buffer
    if (!selection.isEmpty())
    {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(copyText(), QClipboard::Selection);
    }

    m_lastSelection = newSelection;
}

ChatHistoryMessage::ChatHistoryMessage()
    : archived(false), received(true)
{
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

