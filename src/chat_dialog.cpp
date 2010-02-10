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

#include <QContextMenuEvent>
#include <QDebug>
#include <QEvent>
#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStringList>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_dialog.h"
#include "chat_edit.h"
#include "chat_history.h"

ChatDialog::ChatDialog(const QString &jid, QWidget *parent)
    : QWidget(parent),
    chatRemoteJid(jid)
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
    avatarLabel = new QLabel;
    hbox->addWidget(avatarLabel);
    layout->addItem(hbox);

    /* chat history */
    chatHistory = new ChatHistory;
    layout->addWidget(chatHistory);

    /* text edit */
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(send()));
    layout->addSpacing(10);
    layout->addWidget(chatInput);

    setFocusProxy(chatInput);
    setLayout(layout);
    setMinimumWidth(300);

    /* shortcuts for new line */
    connect(new QShortcut(QKeySequence(Qt::AltModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(newLine()));
    connect(new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(newLine()));
}

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
{
    foreach (const QXmppArchiveMessage &msg, chat.messages)
        chatHistory->addMessage(msg, true);
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    QXmppArchiveMessage message;
    message.body = msg.getBody();
    message.local = false;
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);
}

void ChatDialog::newLine()
{
    chatInput->append("");
}

void ChatDialog::send()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    QXmppArchiveMessage message;
    message.body = text;
    message.local = true;
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);

    chatInput->document()->clear();

    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(chatRemoteJid);
    emit sendMessage(msg);
}

void ChatDialog::setLocalName(const QString &name)
{
    chatHistory->setLocalName(name);
}

void ChatDialog::setRemoteAvatar(const QPixmap &avatar)
{
    avatarLabel->setPixmap(avatar);
}

void ChatDialog::setRemoteName(const QString &name)
{
    nameLabel->setText(QString("<b>%1</b><br/>%2")
        .arg(name)
        .arg(chatRemoteJid));
    chatHistory->setRemoteName(name);
}

