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

#ifndef __WILINK_ROOMS_H__
#define __WILINK_ROOMS_H__

#include <QDialog>

#include "QXmppDataForm.h"
#include "QXmppMucIq.h"
#include "model.h"

class ChatClient;
class ChatHistoryModel;
class ChatMessage;
class ChatRosterModel;
class QModelIndex;
class QTableWidget;
class QXmppMessage;
class QXmppMucManager;
class QXmppMucRoom;

class RoomListModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)

public:
    RoomListModel(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    QVariant data(const QModelIndex &index, int role) const;

signals:
    void clientChanged(ChatClient *client);

public slots:
    void addPendingMessage(const QString &jid);
    void clearPendingMessages(const QString &jid);

    void addRoom(const QString &jid);
    void removeRoom(const QString &jid);

private slots:
    void bookmarksReceived();

private:
    ChatClient *m_client;
};

class RoomModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(QXmppMucManager* manager READ manager WRITE setManager NOTIFY managerChanged)
    Q_PROPERTY(QXmppMucRoom* room READ room NOTIFY roomChanged)
    Q_PROPERTY(ChatHistoryModel* historyModel READ historyModel CONSTANT)

public:
    RoomModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    ChatHistoryModel *historyModel() const;

    QString jid() const;
    void setJid(const QString &jid);

    QXmppMucManager *manager() const;
    void setManager(QXmppMucManager *manager);

    QXmppMucRoom *room() const;

signals:
    void jidChanged(const QString &jid);
    void managerChanged(QXmppMucManager *manager);
    void roomChanged(QXmppMucRoom *room);

public slots:
    void bookmark();
    void unbookmark();

private slots:
    void messageReceived(const QXmppMessage &msg);
    void participantAdded(const QString &jid);
    void participantChanged(const QString &jid);
    void participantRemoved(const QString &jid);

private:
    void setRoom(QXmppMucRoom *room);

    ChatHistoryModel *m_historyModel;
    QString m_jid;
    QXmppMucManager *m_manager;
    QXmppMucRoom *m_room;
};

class ChatForm : public QDialog
{
    Q_OBJECT

public:
    ChatForm(const QXmppDataForm &form, QWidget *parent);
    QXmppDataForm form() const;

protected slots:
    void submit();

private:
    QXmppDataForm chatForm;
};

class RoomPermissionDialog : public QDialog
{
    Q_OBJECT

public:
    RoomPermissionDialog(QXmppMucRoom *mucRoom, const QString &defaultJid, QWidget *parent);

protected slots:
    void addMember();
    void removeMember();
    void permissionsReceived(const QList<QXmppMucItem> &permissions);
    void submit();

private:
    void addEntry(const QString &jid, QXmppMucItem::Affiliation affiliation);

    QString m_defaultJid;
    QXmppMucRoom *m_room;
    QTableWidget *m_tableWidget;
};

#endif
