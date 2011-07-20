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

#include "model.h"

class QXmppArchiveChat;
class QUrl;

class ChatClient;
class HistoryModel;
class HistoryModelPrivate;

/** The HistoryMessage class represents the data for a single chat history message.
 */
class HistoryMessage
{
public:
    HistoryMessage();
    QString html(const QString &meName) const;
    bool groupWith(const HistoryMessage &other) const;
    bool isAction() const;

    static void addTransform(const QRegExp &match, const QString &replacement);

    bool archived;
    QString body;
    QDateTime date;
    QString jid;
    bool received;
};

/** The HistoryModel class represents a conversation history with
 *  a given person or chat room.
 */
class HistoryModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

public:
    enum HistoryRole {
        ActionRole = Qt::UserRole + 10,
        BodyRole,
        DateRole,
        FromRole,
        HtmlRole,
        ReceivedRole,
        SelectedRole,
    };

    HistoryModel(QObject *parent = 0);
    void addMessage(const HistoryMessage &message);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    QString jid() const;
    void setJid(const QString &jid);

    // QAbstracItemModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void bottomChanged();
    void bottomAboutToChange();
    void clientChanged(ChatClient *client);
    void jidChanged(const QString &jid);
    void messageReceived(const QString &jid, const QString &text);
    void participantModelChanged(QObject *participantModel);

public slots:
    void clear();
    void select(int from, int to);

private slots:
    void _q_cardChanged();
    void _q_archiveChatReceived(const QXmppArchiveChat &chat);
    void _q_archiveListReceived(const QList<QXmppArchiveChat> &chats);

private:
    friend class HistoryModelPrivate;
    HistoryModelPrivate *d;
};

#endif
