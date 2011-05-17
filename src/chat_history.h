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

#ifndef __WILINK_CHAT_HISTORY_H__
#define __WILINK_CHAT_HISTORY_H__

#include <QDateTime>
#include <QTextCursor>
#include <QTextDocument>

#include "chat_model.h"

class QUrl;

class ChatHistoryModel;
class ChatHistoryModelPrivate;
class ChatRosterModel;

/** The ChatMessage class represents the data for a single chat history message.
 */
class ChatMessage
{
public:
    ChatMessage();
    QString html(const QString &meName) const;
    bool groupWith(const ChatMessage &other) const;
    bool isAction() const;

    static void addTransform(const QRegExp &match, const QString &replacement);

    bool archived;
    QString body;
    QDateTime date;
    QString jid;
    bool received;
};

/** The ChatHistoryModel class represents a conversation history with
 *  a given person or chat room.
 */
class ChatHistoryModel : public ChatModel
{
    Q_OBJECT

public:
    enum HistoryRole {
        ActionRole = Qt::UserRole,
        AvatarRole,
        BodyRole,
        DateRole,
        FromRole,
        JidRole,
        HtmlRole,
        ReceivedRole,
    };

    ChatHistoryModel(QObject *parent = 0);
    void addMessage(const ChatMessage &message);
    ChatRosterModel *rosterModel();
    void setRosterModel(ChatRosterModel *rosterModel);

    // QAbstracItemModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void bottomChanged();
    void bottomAboutToChange();

public slots:
    void clear();

private slots:
    void rosterChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rosterInserted(const QModelIndex &parent, int start, int end);

private:
    friend class ChatHistoryModelPrivate;
    ChatHistoryModelPrivate *d;
};

#endif
