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

ChatMessageWidget::ChatMessageWidget(QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
    bodyText = scene()->addText("");
    bodyText->setParentItem(this);

    dateText = scene()->addText("");
    dateText->setParentItem(this);

    fromText = scene()->addText("");
    fromText->setParentItem(this);
}

void ChatMessageWidget::setBody(const QString &body)
{
    bodyText->setPlainText(body);
}

void ChatMessageWidget::setDate(const QDateTime &datetime)
{
    dateText->setPlainText(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
}

void ChatMessageWidget::setFrom(const QString &from)
{
    fromText->setPlainText(from);
}

void ChatMessageWidget::setGeometry(const QRectF &rect)
{
    fromText->setPos(rect.x(), rect.y());

    dateText->setPos(rect.x() + rect.width() - 105, rect.y());
    dateText->setTextWidth(90);

    bodyText->setPos(rect.x(), rect.y() + 20);
    bodyText->setTextWidth(rect.width());
}

QSizeF ChatMessageWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which)
    {
        case Qt::MinimumSize:
            return QSizeF(10, 10);
        case Qt::PreferredSize:
        {
            QSizeF bodyHint(bodyText->document()->size());
            QSizeF dateHint(dateText->document()->size());
            QSizeF fromHint(fromText->document()->size());

            QSizeF hint(dateHint);
            hint.setWidth(hint.width() + fromHint.width());
            hint.setHeight(hint.height() + dateHint.height());
            if (bodyHint.width() > hint.width())
                hint.setWidth(bodyHint.width());
            return hint;
        }
        default:
            return constraint;
    }
}

#ifdef USE_GRAPHICSVIEW
ChatHistory::ChatHistory(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene;
    setScene(scene);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    obj = new QGraphicsWidget;
    layout = new QGraphicsLinearLayout(Qt::Vertical);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    //obj->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    obj->setLayout(layout);
    scene->addItem(obj);
}

#else

ChatHistory::ChatHistory(QWidget *parent)
    : QTextBrowser(parent)
{
    QFile styleSheet(":/chat.css");
    if (styleSheet.open(QIODevice::ReadOnly))
    {
        document()->setDefaultStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }

    setOpenLinks(false);
    connect(this, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(slotAnchorClicked(const QUrl&)));
}

#endif

void ChatHistory::addMessage(const QXmppArchiveMessage &message)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() == scrollBar->maximum();

    /* position cursor */
    int i = 0;
    while (i < messages.size() && message.datetime > messages.at(i).datetime)
        i++;
    messages.insert(i, message);

#ifdef USE_GRAPHICSVIEW
    ChatMessageWidget *msg = new ChatMessageWidget(obj);
    msg->setBody(message.body);
    msg->setDate(message.datetime.toLocalTime());
    msg->setFrom(message.local ? chatLocalName : chatRemoteName);
    layout->addItem(msg);
#else
    QTextCursor cursor(document()->findBlockByNumber(i * 4));

    /* add message */
    bool showSender = (i == 0 || messages.at(i-1).local != message.local);
    QDateTime datetime = message.datetime.toLocalTime();
    QString dateString(datetime.date() == QDate::currentDate() ? datetime.toString("hh:mm") : datetime.toString("dd MMM hh:mm"));
    const QString type(message.local ? "local": "remote");

    QString html = Qt::escape(message.body);
    html.replace(QRegExp("((ftp|http|https)://[^ ]+)"), "<a href=\"\\1\">\\1</a>");
    cursor.insertHtml(QString(
        "<table cellspacing=\"0\" width=\"100%\">"
        "<tr class=\"title\">"
        "  <td class=\"from %1\">%2</td>"
        "  <td class=\"time %3\" align=\"center\" width=\"100\">%4</td>"
        "</tr>"
        "<tr>"
        "  <td class=\"body\" colspan=\"2\">%5</td>"
        "</tr>"
        "</table>")
        .arg(showSender ? type : "line")
        .arg(showSender ? (message.local ? chatLocalName : chatRemoteName) : "")
        .arg(type)
        .arg(dateString)
        .arg(html));
#endif

    /* scroll to end if we were previous at end */
    if (atEnd)
        scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatHistory::clear()
{
    messages.clear();
#ifdef USE_GRAPHICSVIEW
    // TODO
#else
    QTextBrowser::clear();
#endif
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
#ifdef USE_GRAPHICSVIEW
    QMenu *menu = new QMenu;
#else
    QMenu *menu = createStandardContextMenu();
#endif
    QAction *action = menu->addAction(tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(clear()));
    menu->exec(event->globalPos());
    delete menu;
}

void ChatHistory::resizeEvent(QResizeEvent *e)
{
    QScrollBar *scrollBar = verticalScrollBar();
    bool atEnd = scrollBar->sliderPosition() == scrollBar->maximum();

#ifdef USE_GRAPHICSVIEW
    qreal w = width() - scrollBar->sizeHint().width();
    obj->setMaximumWidth(w);
    obj->setPreferredWidth(w);
    obj->adjustSize();
    QGraphicsView::resizeEvent(e);
#else
    QTextBrowser::resizeEvent(e);
#endif

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

