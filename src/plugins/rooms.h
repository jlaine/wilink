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

#include "QXmppDataForm.h"
#include "QXmppMucIq.h"
#include "model.h"

class ChatClient;
class HistoryModel;
class QModelIndex;
class QXmppMessage;
class QXmppMucManager;
class QXmppMucRoom;

class RoomConfigurationModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QXmppMucRoom* room READ room WRITE setRoom NOTIFY roomChanged)

public:
    RoomConfigurationModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex & parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent) const;

    QXmppMucRoom *room() const;
    void setRoom(QXmppMucRoom *room);

signals:
    void roomChanged(QXmppMucRoom *room);

public slots:
    void save();
    void setValue(int row, const QVariant &value);

private slots:
    void _q_configurationReceived(const QXmppDataForm &configuration);

private:
    enum Role {
        DescriptionRole,
        KeyRole,
        LabelRole,
        TypeRole,
        ValueRole,
    };
    QXmppDataForm m_form;
    QXmppMucRoom *m_room;
};

class RoomListModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)
    Q_PROPERTY(int pendingMessages READ pendingMessages NOTIFY pendingMessagesChanged)

public:
    RoomListModel(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    int pendingMessages() const;

    QVariant data(const QModelIndex &index, int role) const;

signals:
    void clientChanged(ChatClient *client);
    void pendingMessagesChanged();
    void roomAdded(const QString &jid);

public slots:
    void addPendingMessage(const QString &jid);
    void clearPendingMessages(const QString &jid);

    void addRoom(const QString &jid);
    void removeRoom(const QString &jid);

private slots:
    void _q_bookmarksReceived();
    void _q_participantsChanged();
    void _q_roomAdded(QXmppMucRoom *room);

private:
    enum Role {
        ParticipantsRole = ChatModel::UserRole,
    };
    ChatClient *m_client;
};

class RoomModel : public ChatModel
{
    Q_OBJECT
    Q_ENUMS(Role)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(QXmppMucManager* manager READ manager WRITE setManager NOTIFY managerChanged)
    Q_PROPERTY(QXmppMucRoom* room READ room NOTIFY roomChanged)
    Q_PROPERTY(HistoryModel* historyModel READ historyModel CONSTANT)

public:
    enum Role {
        AffiliationRole = ChatModel::UserRole,
    };

    RoomModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    HistoryModel *historyModel() const;

    QString jid() const;
    void setJid(const QString &jid);

    QXmppMucManager *manager() const;
    void setManager(QXmppMucManager *manager);

    QXmppMucRoom *room() const;

signals:
    void jidChanged(const QString &jid);
    void managerChanged(QXmppMucManager *manager);
    void roomChanged(QXmppMucRoom *room);

private slots:
    void messageReceived(const QXmppMessage &msg);
    void participantAdded(const QString &jid);
    void participantChanged(const QString &jid);
    void participantRemoved(const QString &jid);

private:
    void setRoom(QXmppMucRoom *room);

    HistoryModel *m_historyModel;
    QString m_jid;
    QXmppMucManager *m_manager;
    QXmppMucRoom *m_room;
};

class RoomPermissionModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(QXmppMucRoom* room READ room WRITE setRoom NOTIFY roomChanged)

public:
    RoomPermissionModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role) const;

    QXmppMucRoom *room() const;
    void setRoom(QXmppMucRoom *room);

signals:
    void roomChanged(QXmppMucRoom *room);

public slots:
    void setPermission(const QString &jid, int affiliation);
    void removePermission(const QString &jid);
    void save();

private slots:
    void _q_permissionsReceived(const QList<QXmppMucItem> &permissions);

private:
    enum Role {
        AffiliationRole = ChatModel::UserRole,
    };

    QXmppMucRoom *m_room;
};

#endif
