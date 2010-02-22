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

#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>

#include "chat_conversation.h"
#include "chat_edit.h"
#include "chat_history.h"

ChatConversation::ChatConversation(const QString &jid, QWidget *parent)
    : QWidget(parent),
    chatRemoteJid(jid), chatLocalState(QXmppMessage::None)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    /* status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    nameLabel = new QLabel(chatRemoteJid);
    hbox->addSpacing(16);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    iconLabel = new QLabel;
    hbox->addWidget(iconLabel);
    QPushButton *button = new QPushButton;
    button->setFlat(true);
    button->setIcon(QIcon(":/close.png"));
    connect(button, SIGNAL(clicked()), this, SLOT(slotLeave()));
    hbox->addWidget(button);
    layout->addItem(hbox);

    /* chat history */
    chatHistory = new ChatHistory;
    connect(chatHistory, SIGNAL(focused()), this, SLOT(slotFocused()));
    layout->addWidget(chatHistory);

    /* text edit */
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(focused()), this, SLOT(slotFocused()));
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(slotSend()));
    connect(chatInput, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
    layout->addSpacing(10);
    layout->addWidget(chatInput);

    setFocusProxy(chatInput);
    setLayout(layout);
    setMinimumWidth(300);

    /* timers */
    pausedTimer = new QTimer(this);
    pausedTimer->setInterval(30000);
    pausedTimer->setSingleShot(true);
    connect(pausedTimer, SIGNAL(timeout()), this, SLOT(slotPaused()));

    inactiveTimer = new QTimer(this);
    inactiveTimer->setInterval(120000);
    inactiveTimer->setSingleShot(true);
    connect(inactiveTimer, SIGNAL(timeout()), this, SLOT(slotInactive()));

    /* shortcuts for new line */
    connect(new QShortcut(QKeySequence(Qt::AltModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));
    connect(new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));

    /* start inactivity timer */
    inactiveTimer->start();
}

void ChatConversation::join()
{
}

void ChatConversation::leave()
{
    chatLocalState = QXmppMessage::Gone;
    emit localStateChanged(chatLocalState);
}

QString ChatConversation::localName() const
{
    return chatLocalName;
}

QXmppMessage::State ChatConversation::localState() const
{
    return chatLocalState;
}

void ChatConversation::setLocalName(const QString &name)
{
    chatLocalName = name;
}

void ChatConversation::setRemoteName(const QString &name)
{
    chatRemoteName = name;
    setRemoteState(QXmppMessage::None);
}

void ChatConversation::setRemotePixmap(const QPixmap &avatar)
{
    iconLabel->setPixmap(avatar);
}

void ChatConversation::setRemoteState(QXmppMessage::State state)
{
    QString stateName;
    if (state == QXmppMessage::Active)
        stateName = "active";
    else if (state == QXmppMessage::Composing)
        stateName = "composing";
    else if (state == QXmppMessage::Gone)
        stateName = "gone";
    else if (state == QXmppMessage::Inactive)
        stateName = "inactive";
    else if (state == QXmppMessage::Paused)
        stateName = "paused";

    if (!stateName.isEmpty())
        stateName = QString(" (%1)").arg(stateName);

    nameLabel->setText(QString("<b>%1</b>%2<br/>%3")
        .arg(chatRemoteName)
        .arg(stateName)
        .arg(chatRemoteJid));
}

void ChatConversation::slotFocused()
{
    if (chatLocalState != QXmppMessage::Active &&
        chatLocalState != QXmppMessage::Composing &&
        chatLocalState != QXmppMessage::Paused)
    {
        chatLocalState = QXmppMessage::Active;
        emit localStateChanged(chatLocalState);
    }
    // reset inactivity timer
    inactiveTimer->stop();
    inactiveTimer->start();
}

void ChatConversation::slotInactive()
{
    if (chatLocalState != QXmppMessage::Inactive)
    {
        chatLocalState = QXmppMessage::Inactive;
        emit localStateChanged(chatLocalState);
    }
}

void ChatConversation::slotLeave()
{
    emit leave(chatRemoteJid);
}

void ChatConversation::slotNewLine()
{
    chatInput->append("");
}

void ChatConversation::slotPaused()
{
    if (chatLocalState == QXmppMessage::Composing)
    {
        chatLocalState = QXmppMessage::Paused;
        emit localStateChanged(chatLocalState);
    }
}

void ChatConversation::slotSend()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    chatLocalState = QXmppMessage::Active;
    chatInput->document()->clear();
    sendMessage(text);
}

void ChatConversation::slotTextChanged()
{
    QString text = chatInput->document()->toPlainText();
    if (!text.isEmpty())
    {
        if (chatLocalState != QXmppMessage::Composing)
        {
            chatLocalState = QXmppMessage::Composing;
            emit localStateChanged(chatLocalState);
        }
        pausedTimer->start();
    } else {
        if (chatLocalState != QXmppMessage::Active)
        {
            chatLocalState = QXmppMessage::Active;
            emit localStateChanged(chatLocalState);
        }
        pausedTimer->stop();
    }
    // reset inactivity timer
    inactiveTimer->stop();
    inactiveTimer->start();
}

