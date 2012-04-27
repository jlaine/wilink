/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __WILINK_CHAT_DIALOG_H__
#define __WILINK_CHAT_DIALOG_H__

#include "QXmppMessage.h"

class ChatClient;
class HistoryModel;
class RosterModel;

class Conversation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(HistoryModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(int localState READ localState WRITE setLocalState NOTIFY localStateChanged)
    Q_PROPERTY(int remoteState READ remoteState NOTIFY remoteStateChanged)

public:
    Conversation(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    HistoryModel *historyModel() const;

    QString jid() const;
    void setJid(const QString &jid);

    int localState() const;
    void setLocalState(int state);

    int remoteState() const;

    RosterModel *rosterModel() const;
    void setRosterModel(RosterModel *rosterModel);

signals:
    void clientChanged(ChatClient *client);
    void localStateChanged(int localState);
    void jidChanged(const QString &jid);
    void remoteStateChanged(int remoteState);
    void rosterModelChanged(RosterModel *rosterModel);

public slots:
    bool sendMessage(const QString &text);

private slots:
    void messageReceived(const QXmppMessage &msg);

private:
    ChatClient *m_client;
    HistoryModel *m_historyModel;
    QXmppMessage::State m_localState;
    QString m_jid;
    QXmppMessage::State m_remoteState;
};

#endif
