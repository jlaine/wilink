/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include <QAction>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBrowser>
#include <QTimer>

#include "QXmppClient.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_search.h"
#include "console.h"
#include "chat_utils.h"

#define CONSOLE_ROSTER_ID "0_console"

/** Constructs a ConsolePanel.
 */
ConsolePanel::ConsolePanel(Chat *chatWindow, QXmppLogger *logger, QWidget *parent)
    : ChatPanel(parent),
    connected(false),
    currentLogger(logger),
    m_window(chatWindow)
{
    bool check;
    setWindowIcon(QIcon(":/options.png"));
    setWindowTitle(tr("Debugging console"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());

    browser = new QTextBrowser;
    layout->addWidget(browser);
    highlighter = new Highlighter(browser->document());

    // search box
    searchBar = new ChatSearchBar;
    searchBar->hide();
    connect(searchBar, SIGNAL(find(QString, QTextDocument::FindFlags, bool)),
        this, SLOT(slotFind(QString, QTextDocument::FindFlags, bool)));
    connect(this, SIGNAL(findFinished(bool)),
        searchBar, SLOT(findFinished(bool)));
    layout->addWidget(searchBar);

    // actions
    startAction = addAction(QIcon(":/start.png"), tr("Start"));
    connect(startAction, SIGNAL(triggered()), this, SLOT(slotStart()));

    stopAction = addAction(QIcon(":/stop.png"), tr("Stop"));
    connect(stopAction, SIGNAL(triggered()), this, SLOT(slotStop()));

    QAction *clearAction = addAction(QIcon(":/close.png"), tr("Clear"));
    connect(clearAction, SIGNAL(triggered()), browser, SLOT(clear()));

    setLayout(layout);

    // connect signals
    connect(this, SIGNAL(findPanel()), searchBar, SLOT(activate()));
    connect(this, SIGNAL(findAgainPanel()), searchBar, SLOT(findNext()));
    connect(this, SIGNAL(hidePanel()), this, SLOT(slotStop()));
    connect(this, SIGNAL(showPanel()), this, SLOT(slotStart()));

    // register shortcut
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_D), m_window);
    check = connect(shortcut, SIGNAL(activated()),
                    this, SIGNAL(showPanel()));
    Q_ASSERT(check);

#if 0
    // register action
    QAction *action = m_window->addAction(windowIcon(), windowTitle());
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_D));
    action->setVisible(false);
    check = connect(action, SIGNAL(triggered()),
                    this, SIGNAL(showPanel()));
    Q_ASSERT(check);
#endif
}

void ConsolePanel::message(QXmppLogger::MessageType type, const QString &msg)
{
    QColor color;
    QString message;
    if (type == QXmppLogger::SentMessage || type == QXmppLogger::ReceivedMessage)
    {
        color = (type == QXmppLogger::SentMessage) ? QColor(0xcc, 0xcc, 0xff) : QColor(0xcc, 0xff, 0xcc);
        message = indentXml(msg);
    }
    else
    {
        color = (type == QXmppLogger::WarningMessage) ? QColor(0xff, 0x95, 0x95) : Qt::white;
        message = msg;
    }

    if (!message.isEmpty())
    {
        QScrollBar *scrollBar = browser->verticalScrollBar();
        const bool atEnd = scrollBar->sliderPosition() > (scrollBar->maximum() - 10);

        QTextCursor cursor = browser->textCursor();
        cursor.movePosition(QTextCursor::End);
        QTextCharFormat fmt = cursor.blockCharFormat();
        fmt.setBackground(QBrush(color));
        cursor.setBlockCharFormat(fmt);
        cursor.insertText(message + "\n");

        if (atEnd)
            scrollBar->setSliderPosition(scrollBar->maximum());
    }
}

/** Find the given text in the console history.
 *
 * @param needle
 * @param flags
 * @param changed
 */
void ConsolePanel::slotFind(const QString &needle, QTextDocument::FindFlags flags, bool changed)
{
    // handle empty search
    if (needle.isEmpty())
    {
        emit findFinished(true);
        return;
    }

    // position cursor and perform search
    QTextCursor cursor = browser->textCursor();
    if (changed)
        cursor.setPosition(cursor.anchor());
    cursor = browser->document()->find(needle, cursor, flags);

    // if search fails, retry from start of document
    if (cursor.isNull())
    {
        QTextCursor start(browser->document());
        if (flags && QTextDocument::FindBackward)
            start.movePosition(QTextCursor::End);
        cursor = browser->document()->find(needle, start, flags);
    }

    // process results
    if (!cursor.isNull())
    {
        browser->setTextCursor(cursor);
        browser->ensureCursorVisible();
        emit findFinished(true);
    } else {
        emit findFinished(false);
    }
}

void ConsolePanel::slotStop()
{
    if (!connected)
        return;
    stopAction->setVisible(false);
    disconnect(currentLogger, SIGNAL(message(QXmppLogger::MessageType,QString)), this, SLOT(message(QXmppLogger::MessageType,QString)));
    connected = false;
    startAction->setVisible(true);
}

void ConsolePanel::slotStart()
{
    if (connected)
        return;
    startAction->setVisible(false);
    connect(currentLogger, SIGNAL(message(QXmppLogger::MessageType,QString)), this, SLOT(message(QXmppLogger::MessageType,QString)));
    connected = true;
    stopAction->setVisible(true);
}

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    tagFormat.setForeground(Qt::darkBlue);
    tagFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegExp("</?([a-z:]+)[^>]*>");
    rule.format = tagFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegExp("'[^']*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            QStringList captures = expression.capturedTexts();
            if (captures.size() > 1)
                setFormat(expression.pos(1), captures[1].size(), rule.format);
            else
                setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}

// PLUGIN

class ConsolePlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Debugging console"; };
};

bool ConsolePlugin::initialize(Chat *chat)
{
    bool check;

    /* register panel */
    ConsolePanel *console = new ConsolePanel(chat, chat->client()->logger());
    console->setObjectName(CONSOLE_ROSTER_ID);
    chat->addPanel(console);

#ifdef WILINK_EMBEDDED
    /* add menu entry */
    QList<QAction*> actions = chat->fileMenu()->actions();
    QAction *firstAction = actions.isEmpty() ? 0 : actions.first();
    QAction *action = new QAction(console->windowIcon(), console->windowTitle(), chat->fileMenu());
    chat->fileMenu()->insertAction(firstAction, action);
    check = connect(action, SIGNAL(triggered()),
                    console, SIGNAL(showPanel()));
    Q_ASSERT(check);
#endif
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(console, ConsolePlugin)
