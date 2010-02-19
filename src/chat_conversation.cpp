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

#include "chat_conversation.h"
#include "chat_edit.h"
#include "chat_history.h"

ChatConversation::ChatConversation(const QString &jid, QWidget *parent)
    : QWidget(parent),
    chatRemoteJid(jid), state(QXmppMessage::None)
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
    layout->addWidget(chatHistory);

    /* text edit */
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(slotSend()));
    connect(chatInput, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
    layout->addSpacing(10);
    layout->addWidget(chatInput);

    setFocusProxy(chatInput);
    setLayout(layout);
    setMinimumWidth(300);

    /* shortcuts for new line */
    connect(new QShortcut(QKeySequence(Qt::AltModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));
    connect(new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));
}

void ChatConversation::join()
{
}

void ChatConversation::leave()
{
}

QString ChatConversation::localName() const
{
    return chatLocalName;
}

void ChatConversation::setLocalName(const QString &name)
{
    chatLocalName = name;
}

void ChatConversation::setRemotePixmap(const QPixmap &avatar)
{
    iconLabel->setPixmap(avatar);
}

void ChatConversation::setRemoteName(const QString &name)
{
    chatRemoteName = name;
    nameLabel->setText(QString("<b>%1</b><br/>%2")
        .arg(chatRemoteName)
        .arg(chatRemoteJid));
}

void ChatConversation::slotLeave()
{
    emit leave(chatRemoteJid);
}

void ChatConversation::slotNewLine()
{
    chatInput->append("");
}

void ChatConversation::slotSend()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    state = QXmppMessage::Active;
    chatInput->document()->clear();
    sendMessage(text);
}

void ChatConversation::slotTextChanged()
{
    QString text = chatInput->document()->toPlainText();
    if (!text.isEmpty())
    {
        if (state != QXmppMessage::Composing)
        {
            state = QXmppMessage::Composing;
            emit stateChanged(state);
        }
    } else {
        if (state != QXmppMessage::Active)
        {
            state = QXmppMessage::Active;
            emit stateChanged(state);
        }
    }
}

