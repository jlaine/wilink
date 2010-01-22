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
#include <QDesktopServices>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextBrowser>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_dialog.h"
#include "chat_edit.h"

ChatDialog::ChatDialog(const QString &jid, const QString &name, QWidget *parent)
    : QWidget(parent), chatRemoteJid(jid), chatRemoteName(name)
{
    chatLocalName = tr("Me");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    /* status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    statusLabel = new QLabel;
    hbox->addWidget(statusLabel);
    QLabel *nameLabel = new QLabel(chatRemoteJid);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    avatarLabel = new QLabel;
    hbox->addWidget(avatarLabel);
    layout->addItem(hbox);

    /* chat history */
    chatHistory = new ChatHistory;
    layout->addWidget(chatHistory);

    /* text edit */
    hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(send()));
    hbox->addWidget(chatInput);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowTitle(tr("Chat with %1").arg(chatRemoteName));
}

void ChatDialog::addMessage(const QString &text, bool local, const QDateTime &datetime)
{
    QString html = text;
    html.replace(QRegExp("((ftp|http|https)://[^ ]+)"), "<a href=\"\\1\">\\1</a>");
    chatHistory->append(QString(
        "<table cellspacing=\"0\" width=\"100%\">"
        "<tr style=\"background-color: %1\">"
        "  <td>%2</td>"
        "  <td align=\"right\">%3</td>"
        "</tr>"
        "<tr>"
        "  <td colspan=\"2\">%4</td>"
        "</tr>"
        "</table>")
        .arg(local ? "#dbdbdb" : "#b6d4ff")
        .arg(local ? chatLocalName : chatRemoteName)
        .arg(QDateTime::currentDateTime().toString("hh:mm"))
        .arg(html));
}

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
{
    foreach (const QXmppArchiveMessage &msg, chat.messages)
        addMessage(msg.body, msg.local, msg.datetime);
}

/** When the window is activated, pass focus to the input line.
 */
void ChatDialog::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
        chatInput->setFocus();
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    addMessage(msg.getBody(), false, QDateTime::currentDateTime());
}

void ChatDialog::send()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    addMessage(text, true, QDateTime::currentDateTime());

    /* scroll to end, but don't touch cursor */
    QScrollBar *scrollBar = chatHistory->verticalScrollBar();
    scrollBar->setSliderPosition(scrollBar->maximum());

    chatInput->document()->clear();
    emit sendMessage(chatRemoteJid, text);
}

void ChatDialog::setAvatar(const QPixmap &avatar)
{
    avatarLabel->setPixmap(avatar);
}

void ChatDialog::setStatus(const QString &status)
{
    statusLabel->setPixmap(status);
}

ChatHistory::ChatHistory(QWidget *parent)
    : QTextBrowser(parent)
{
    setOpenLinks(false);
    connect(this, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(slotAnchorClicked(const QUrl&)));
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    QAction *action = menu->addAction(tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(clear()));
    menu->exec(event->globalPos());
    delete menu;
}

void ChatHistory::slotAnchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}

