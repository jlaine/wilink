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

#include <QApplication>
#include <QKeyEvent>
#include <QTimer>

#include "chat_edit.h"

class ChatEditPrivate
{
public:
    int maxHeight;
    QTimer *inactiveTimer;
    QTimer *pausedTimer;
    QXmppMessage::State state;
};

ChatEdit::ChatEdit(int maxheight, QWidget* parent)
    : QTextEdit(parent),
    d(new ChatEditPrivate)
{
    d->maxHeight = maxheight;
    d->state = QXmppMessage::None;

    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    setAcceptRichText(false);
    setSizePolicy(sizePolicy);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

    /* timers */
    d->pausedTimer = new QTimer(this);
    d->pausedTimer->setInterval(30000);
    d->pausedTimer->setSingleShot(true);
    connect(d->pausedTimer, SIGNAL(timeout()), this, SLOT(slotPaused()));

    d->inactiveTimer = new QTimer(this);
    d->inactiveTimer->setInterval(120000);
    d->inactiveTimer->setSingleShot(true);
    connect(d->inactiveTimer, SIGNAL(timeout()), this, SLOT(slotInactive()));

    /* start inactivity timer */
    d->inactiveTimer->start();
}

ChatEdit::~ChatEdit()
{
    delete d;
}

void ChatEdit::focusInEvent(QFocusEvent *e)
{
    QTextEdit::focusInEvent(e);

    if (d->state != QXmppMessage::Active &&
        d->state != QXmppMessage::Composing &&
        d->state != QXmppMessage::Paused)
    {
        d->state = QXmppMessage::Active;
        emit stateChanged(d->state);
    }
    // reset inactivity timer
    d->inactiveTimer->stop();
    d->inactiveTimer->start();

    emit focused();
}

void ChatEdit::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if (QApplication::keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier))
        {
            QTextCursor cursor = textCursor();
            cursor.insertBlock();
        } else
            emit returnPressed();
    }
    else if (e->key() == Qt::Key_Tab)
        emit tabPressed();
    else
        QTextEdit::keyPressEvent(e);
}

QSize ChatEdit::minimumSizeHint() const
{
    QSize sizeHint = QTextEdit::minimumSizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, d->maxHeight));
    return sizeHint;
}

void ChatEdit::onTextChanged()
{
    static int oldHeight = 0;
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    if (myHeight != oldHeight)
        updateGeometry();
    oldHeight = myHeight;
}

void ChatEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    updateGeometry();
}

QSize ChatEdit::sizeHint() const
{
    QSize sizeHint = QTextEdit::sizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, d->maxHeight));
    return sizeHint;
}

QXmppMessage::State ChatEdit::state() const
{
    return d->state;
}

void ChatEdit::slotInactive()
{
    if (d->state != QXmppMessage::Inactive)
    {
        d->state = QXmppMessage::Inactive;
        emit stateChanged(d->state);
    }
}

void ChatEdit::slotPaused()
{
    if (d->state == QXmppMessage::Composing)
    {
        d->state = QXmppMessage::Paused;
        emit stateChanged(d->state);
    }
}

