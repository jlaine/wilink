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

#include <QLayout>
#include <QTextBrowser>

#include "chat_console.h"

ChatConsole::ChatConsole(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Debugging console"));

    QVBoxLayout *layout = new QVBoxLayout;
    browser = new QTextBrowser;
    layout->addWidget(browser);
    setLayout(layout);
}

void ChatConsole::message(QXmppLogger::MessageType type, const QString &msg)
{
    if (type == QXmppLogger::DebugMessage)
        return;

    QString color;
    if (type == QXmppLogger::SentMessage)
        color = "green";
    else if (type == QXmppLogger::ReceivedMessage)
        color = "blue";
    else if (type == QXmppLogger::WarningMessage)
        color = "red";
    else
        color = "black";
    
    browser->append(QString("<font color=\"%1\">%2</font>").arg(color, Qt::escape(msg)));
}
