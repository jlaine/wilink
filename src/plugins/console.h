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

#ifndef __WILINK_CONSOLE_H__
#define __WILINK_CONSOLE_H__

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QVector>

#include "QXmppLogger.h"

#include "chat_panel.h"

class ChatSearchBar;
class Highlighter;
class QPushButton;
class QTextBrowser;

/** The ConsolePanel class represents a panel for display debugging
 *  information.
 */
class ConsolePanel : public ChatPanel
{
    Q_OBJECT

public:
    ConsolePanel(QXmppLogger *logger, QWidget *parent = 0);

signals:
    void findFinished(bool found);

private slots:
    void slotFind(const QString &needle, QTextDocument::FindFlags flags, bool changed);
    void slotStart();
    void slotStop();
    void message(QXmppLogger::MessageType type, const QString &msg);

private:
    QTextBrowser *browser;
    bool connected;
    QXmppLogger *currentLogger;
    ChatSearchBar *searchBar;
    Highlighter *highlighter;
    QPushButton *startButton;
    QPushButton *stopButton;
};

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat tagFormat;
    QTextCharFormat quotationFormat;
};

#endif
