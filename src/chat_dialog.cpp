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

#include <QDebug>
#include <QDateTime>
#include <QDesktopServices>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTextBrowser>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_dialog.h"

ChatDialog::ChatDialog(const QString &jid, const QString &name, QWidget *parent)
    : QWidget(parent), chatRemoteJid(jid), chatRemoteName(name)
{
    chatLocalName = tr("Me");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *imageLabel = new QLabel;
    hbox->addWidget(imageLabel);
    layout->addItem(hbox);

    chatHistory = new QTextBrowser;
    chatHistory->setOpenLinks(false);
    connect(chatHistory, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(anchorClicked(const QUrl&)));
    layout->addWidget(chatHistory);

    hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);
    chatInput = new QLineEdit;
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(send()));
    hbox->addWidget(chatInput);
    layout->addItem(hbox);

    setLayout(layout);
    setWindowTitle(tr("Chat with %1").arg(jid));
    chatInput->setFocus();
}

void ChatDialog::addMessage(const QString &text, bool local)
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

void ChatDialog::anchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    addMessage(msg.getBody(), false);
}

void ChatDialog::send()
{
    QString text = chatInput->text();
    if (text.isEmpty())
        return;

    addMessage(text, true);
    chatHistory->ensureCursorVisible();
    chatInput->clear();
    emit sendMessage(chatRemoteJid, text);
}

void ChatDialog::vCardReceived(const QXmppVCard& vcard)
{

}

