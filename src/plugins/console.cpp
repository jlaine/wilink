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

#include <QDateTime>

#include "console.h"
#include "utils.h"

enum LogRole {
    ContentRole = ChatModel::UserRole,
    DateRole,
    TypeRole,
};

class LogItem : public ChatModelItem
{
public:
    QString content;
    QDateTime date;
    QXmppLogger::MessageType type;
};

LogModel::LogModel(QObject *parent)
    : ChatModel(parent),
    m_enabled(true),
    m_logger(0)
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ContentRole, "content");
    roleNames.insert(DateRole, "date");
    roleNames.insert(TypeRole, "type");
    setRoleNames(roleNames);
}

void LogModel::clear()
{
    removeRows(0, rootItem->children.size());
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    LogItem *item = static_cast<LogItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == ContentRole)
        return item->content;
    else if (role == DateRole)
        return item->date;
    else if (role == TypeRole)
        return item->type;

    return QVariant();
}

bool LogModel::enabled() const
{
    return m_enabled;
}

void LogModel::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }
}

QXmppLogger *LogModel::logger() const
{
    return m_logger;
}

void LogModel::setLogger(QXmppLogger *logger)
{
    if (m_logger != logger) {
        if (m_logger)
            disconnect(m_logger, SIGNAL(message(QXmppLogger::MessageType,QString)),
                       this, SLOT(messageReceived(QXmppLogger::MessageType,QString)));
        if (logger)
            connect(logger, SIGNAL(message(QXmppLogger::MessageType,QString)),
                    this, SLOT(messageReceived(QXmppLogger::MessageType,QString)));

        m_logger = logger;
        emit loggerChanged(m_logger);
    }
}

void LogModel::messageReceived(QXmppLogger::MessageType type, const QString &msg)
{
    if (!m_enabled)
        return;

    LogItem *item = new LogItem;
    item->content = msg;
    if (type == QXmppLogger::SentMessage || type == QXmppLogger::ReceivedMessage)
        item->content = indentXml(msg);
    else
        item->content = msg;
    item->date = QDateTime::currentDateTime();
    item->type = type;
    addItem(item, rootItem, rootItem->children.size());
}

#if 0
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

class Highlighter : public QSyntaxHighlighter
{
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

#endif
