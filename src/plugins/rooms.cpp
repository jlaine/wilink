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

#include <QCheckBox>
#include <QComboBox>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QUrl>

#include "QXmppBookmarkManager.h"
#include "QXmppBookmarkSet.h"
#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"

#include "application.h"
#include "client.h"
#include "history.h"
#include "rooms.h"
#include "roster.h"

RoomConfigurationModel::RoomConfigurationModel(QObject *parent)
    : QAbstractListModel(parent),
    m_room(0)
{
    QHash<int, QByteArray> names;
    names.insert(DescriptionRole, "description");
    names.insert(KeyRole, "key");
    names.insert(LabelRole, "label");
    names.insert(TypeRole, "type");
    names.insert(ValueRole, "value");
    setRoleNames(names);
}

QVariant RoomConfigurationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QXmppDataForm::Field field = m_form.fields()[index.row()];
    if (role == DescriptionRole)
        return field.description();
    else if (role == KeyRole)
        return field.key();
    else if (role == LabelRole)
        return field.label();
    else if (role == TypeRole)
        return field.type();
    else if (role == ValueRole)
        return field.value();

    return QVariant();
}

QModelIndex RoomConfigurationModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QXmppMucRoom *RoomConfigurationModel::room() const
{
    return m_room;
}

void RoomConfigurationModel::setRoom(QXmppMucRoom *room)
{
    if (room != m_room) {
        bool check;

        if (m_room)
            m_room->disconnect(this);

        if (m_form.fields().size() > 0) {
            beginRemoveRows(QModelIndex(), 0, m_form.fields().size() - 1);
            m_form = QXmppDataForm();
            endRemoveRows();
        } else {
            m_form = QXmppDataForm();
        }
        m_room = room;

        if (m_room) {
            bool check;
            check = connect(m_room, SIGNAL(configurationReceived(QXmppDataForm)),
                            this, SLOT(_q_configurationReceived(QXmppDataForm)));
            Q_ASSERT(check);
            Q_UNUSED(check);

            m_room->requestConfiguration();
        }

        emit roomChanged(m_room);
    }
}

int RoomConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_form.fields().size();
}

void RoomConfigurationModel::save()
{
    if (m_room) {
        m_room->setConfiguration(m_form);
    }
}

void RoomConfigurationModel::setValue(int row, const QVariant &value)
{
    if (row > 0 && row <  m_form.fields().size() &&
        value != m_form.fields()[row].value()) {
        m_form.fields()[row].setValue(value);
        emit dataChanged(index(row), index(row));
    }
}

void RoomConfigurationModel::_q_configurationReceived(const QXmppDataForm &configuration)
{
    if (!m_form.fields().size()) {
        beginInsertRows(QModelIndex(), 0, configuration.fields().size() - 1);
        m_form = configuration;
        m_form.setType(QXmppDataForm::Submit);
        endInsertRows();
    }
}

class RoomListItem : public ChatModelItem
{
public:
    QString jid;
    int messages;
};

RoomListModel::RoomListModel(QObject *parent)
    : ChatModel(parent),
    m_client(0)
{
}

void RoomListModel::bookmarksReceived()
{
    Q_ASSERT(m_client);

    QXmppBookmarkManager *bookmarkManager = m_client->findExtension<QXmppBookmarkManager>();
    Q_ASSERT(bookmarkManager);

    // join rooms marked as "autojoin"
    const QXmppBookmarkSet &bookmarks = bookmarkManager->bookmarks();
    foreach (const QXmppBookmarkConference &conference, bookmarks.conferences()) {
        if (conference.autoJoin())
            addRoom(conference.jid());
    }
}

QVariant RoomListModel::data(const QModelIndex &index, int role) const
{
    RoomListItem *item = static_cast<RoomListItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == ChatModel::AvatarRole) {
        return QUrl("qrc:/chat.png");
    } else if (role == ChatModel::JidRole) {
        return item->jid;
    } else if (role == ChatModel::MessagesRole) {
        return item->messages;
    } else if (role == ChatModel::NameRole) {
        return jidToUser(item->jid);
    }

    return QVariant();
}

ChatClient *RoomListModel::client() const
{
    return m_client;
}

void RoomListModel::setClient(ChatClient *client)
{
    if (client != m_client) {

        m_client = client;

        if (m_client) {
            bool check;

            // add extension
            QXmppBookmarkManager *bookmarkManager = client->findExtension<QXmppBookmarkManager>();
            if (!bookmarkManager) {
                bookmarkManager = new QXmppBookmarkManager(client);
                client->addExtension(bookmarkManager);
            }

            // connect signals
            check = connect(bookmarkManager, SIGNAL(bookmarksReceived(QXmppBookmarkSet)),
                            this, SLOT(bookmarksReceived()));
            Q_ASSERT(check);
        }

        emit clientChanged(m_client);
    }
}

void RoomListModel::addPendingMessage(const QString &jid)
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomListItem *item = static_cast<RoomListItem*>(ptr);
        if (item->jid == jid) {
            item->messages++;
            emit dataChanged(createIndex(item), createIndex(item));
            break;
        }
    }
}

void RoomListModel::clearPendingMessages(const QString &jid)
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomListItem *item = static_cast<RoomListItem*>(ptr);
        if (item->jid == jid) {
            if (item->messages > 0) {
                item->messages = 0;
                emit dataChanged(createIndex(item), createIndex(item));
            }
            break;
        }
    }
}

void RoomListModel::addRoom(const QString &jid)
{
    if (jid.isEmpty())
        return;

    // add room to list
    int row = rootItem->children.size();
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomListItem *item = static_cast<RoomListItem*>(ptr);
        if (item->jid == jid) {
            return;
        } else if (item->jid.compare(jid, Qt::CaseInsensitive) > 0) {
            row = item->row();
            break;
        }
    }

    RoomListItem *item = new RoomListItem;
    item->jid = jid;
    item->messages = 0;
    addItem(item, rootItem, row);
    emit roomAdded(item->jid);

    // update bookmarks
    if (m_client) {
        QXmppBookmarkManager *bookmarkManager = m_client->findExtension<QXmppBookmarkManager>();
        Q_ASSERT(bookmarkManager);

        // find bookmark
        QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
        QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
        foreach (const QXmppBookmarkConference &conference, conferences) {
            if (conference.jid() == jid)
                return;
        }

        // add bookmark
        QXmppBookmarkConference conference;
        conference.setAutoJoin(true);
        conference.setJid(jid);
        conferences << conference;
        bookmarks.setConferences(conferences);
        bookmarkManager->setBookmarks(bookmarks);
    }
}

void RoomListModel::removeRoom(const QString &jid)
{
    if (jid.isEmpty())
        return;

    // remove room from list
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomListItem *item = static_cast<RoomListItem*>(ptr);
        if (item->jid == jid) {
            removeItem(item);
            break;
        }
    }

    // update bookmarks
    if (m_client) {
        QXmppBookmarkManager *bookmarkManager = m_client->findExtension<QXmppBookmarkManager>();
        Q_ASSERT(bookmarkManager);

        // find bookmark
        QXmppBookmarkSet bookmarks = bookmarkManager->bookmarks();
        QList<QXmppBookmarkConference> conferences = bookmarks.conferences();
        for (int i = 0; i < conferences.size(); ++i) {
            if (conferences.at(i).jid() == jid) {
                // remove bookmark
                conferences.removeAt(i);
                bookmarks.setConferences(conferences);
                bookmarkManager->setBookmarks(bookmarks);
                break;
            }
        }
    }
}

class ChatRoomItem : public ChatModelItem
{
public:
    QString jid;
    QXmppPresence::Status::Type status;
};

RoomModel::RoomModel(QObject *parent)
    : ChatModel(parent),
    m_room(0)
{
    m_historyModel = new HistoryModel(this);

    connect(VCardCache::instance(), SIGNAL(cardChanged(QString)),
            this, SLOT(participantChanged(QString)));
}

QVariant RoomModel::data(const QModelIndex &index, int role) const
{
    ChatRoomItem *item = static_cast<ChatRoomItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == ChatModel::AvatarRole) {
        return VCardCache::instance()->imageUrl(item->jid);
    } else if (role == ChatModel::JidRole) {
        return item->jid;
    } else if (role == ChatModel::NameRole) {
        return jidToResource(item->jid);
    }

    return QVariant();
}

HistoryModel *RoomModel::historyModel() const
{
    return m_historyModel;
}

QString RoomModel::jid() const
{
    return m_jid;
}

void RoomModel::setJid(const QString &jid)
{
    if (jid != m_jid) {
        m_jid = jid;

        if (m_manager && !m_jid.isEmpty())
            setRoom(m_manager->addRoom(m_jid));
        else
            setRoom(0);

        emit jidChanged(m_jid);
    }
}

QXmppMucManager *RoomModel::manager() const
{
    return m_manager;
}

void RoomModel::setManager(QXmppMucManager *manager)
{
    if (manager != m_manager) {
        m_manager = manager;

        if (m_manager && !m_jid.isEmpty())
            setRoom(m_manager->addRoom(m_jid));
        else
            setRoom(0);

        emit managerChanged(m_manager);
    }
}

void RoomModel::messageReceived(const QXmppMessage &msg)
{
    Q_ASSERT(m_room);
    if (msg.body().isEmpty())
        return;

    // handle message body
    HistoryMessage message;
    message.archived = !m_room->isJoined();
    message.body = msg.body();
    message.date = msg.stamp();
    if (!message.date.isValid()) {
        // FIXME: restore this!
        // chat->client()->serverTime();
        message.date = QDateTime::currentDateTime();
    }
    message.jid = msg.from();
    message.received = jidToResource(msg.from()) != m_room->nickName();
    m_historyModel->addMessage(message);
}

void RoomModel::participantAdded(const QString &jid)
{
    Q_ASSERT(m_room);
    //qDebug("participant added %s", qPrintable(jid));

    int row = rootItem->children.size();
    foreach (ChatModelItem *ptr, rootItem->children) {
        ChatRoomItem *item = static_cast<ChatRoomItem*>(ptr);
        if (item->jid == jid) {
            qWarning("participant added twice %s", qPrintable(jid));
            return;
        } else if (item->jid.compare(jid, Qt::CaseInsensitive) > 0) {
            row = item->row();
            break;
        }
    }

    ChatRoomItem *item = new ChatRoomItem;
    item->jid = jid;
    addItem(item, rootItem, row);
}

void RoomModel::participantChanged(const QString &jid)
{
    Q_ASSERT(m_room);
    //qDebug("participant changed %s", qPrintable(jid));

    foreach (ChatModelItem *ptr, rootItem->children) {
        ChatRoomItem *item = static_cast<ChatRoomItem*>(ptr);
        if (item->jid == jid) {
            item->status = m_room->participantPresence(jid).status().type();
            emit dataChanged(createIndex(item), createIndex(item));
            break;
        }
    }
}

void RoomModel::participantRemoved(const QString &jid)
{
    Q_ASSERT(m_room);
    //qDebug("participant removed %s", qPrintable(jid));

    foreach (ChatModelItem *ptr, rootItem->children) {
        ChatRoomItem *item = static_cast<ChatRoomItem*>(ptr);
        if (item->jid == jid) {
            removeRow(item->row());
            break;
        }
    }
}

QXmppMucRoom *RoomModel::room() const
{
    return m_room;
}

void RoomModel::setRoom(QXmppMucRoom *room)
{
    bool check;

    if (room == m_room)
        return;

    // disconnect signals
    if (m_room)
        m_room->disconnect(this);

    m_room = room;

    // connect signals
    if (m_room) {
        check = connect(m_room, SIGNAL(messageReceived(QXmppMessage)),
                        this, SLOT(messageReceived(QXmppMessage)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantAdded(QString)),
                        this, SLOT(participantAdded(QString)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantChanged(QString)),
                        this, SLOT(participantChanged(QString)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantRemoved(QString)),
                        this, SLOT(participantRemoved(QString)));
        Q_ASSERT(check);
    }

    emit roomChanged(m_room);
}

class RoomPermissionItem : public ChatModelItem
{
public:
    QString jid;
    int affiliation;
};

RoomPermissionModel::RoomPermissionModel(QObject *parent)
    : ChatModel(parent),
    m_room(0)
{
    QHash<int, QByteArray> names;
    names.insert(AffiliationRole, "affiliation");
    names.insert(JidRole, "jid");
    setRoleNames(names);
}

void RoomPermissionModel::setPermission(const QString &jid, int affiliation)
{
    // add room to list
    int row = rootItem->children.size();
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomPermissionItem *item = static_cast<RoomPermissionItem*>(ptr);
        if (item->jid == jid) {
            if (affiliation != item->affiliation) {
                item->affiliation = affiliation;
                emit dataChanged(createIndex(item), createIndex(item));
            }
            return;
        } else if (item->jid.compare(jid, Qt::CaseInsensitive) > 0) {
            row = item->row();
            break;
        }
    }

    RoomPermissionItem *item = new RoomPermissionItem;
    item->affiliation = affiliation;
    item->jid = jid;
    addItem(item, rootItem, row);
}

void RoomPermissionModel::removePermission(const QString &jid)
{
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomPermissionItem *item = static_cast<RoomPermissionItem*>(ptr);
        if (item->jid == jid) {
            removeItem(item);
            return;
        }
    }
}

QVariant RoomPermissionModel::data(const QModelIndex &index, int role) const
{
    RoomPermissionItem *item = static_cast<RoomPermissionItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AffiliationRole) {
        return item->affiliation;
    } else if (role == ChatModel::JidRole) {
        return item->jid;
    }

    return QVariant();
}

QXmppMucRoom *RoomPermissionModel::room() const
{
    return m_room;
}

void RoomPermissionModel::setRoom(QXmppMucRoom *room)
{
    if (room != m_room) {
        bool check;

        if (m_room)
            m_room->disconnect(this);

        m_room = room;
        removeRows(0, rootItem->children.size());

        if (m_room) {
            bool check;
            check = connect(m_room, SIGNAL(permissionsReceived(QList<QXmppMucItem>)),
                            this, SLOT(_q_permissionsReceived(QList<QXmppMucItem>)));
            Q_ASSERT(check);
            Q_UNUSED(check);

            m_room->requestPermissions();
        }

        emit roomChanged(m_room);
    }
}

void RoomPermissionModel::save()
{
    if (!m_room)
        return;

    QList<QXmppMucItem> permissions;
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomPermissionItem *item = static_cast<RoomPermissionItem*>(ptr);

        QXmppMucItem mucItem;
        mucItem.setAffiliation(static_cast<QXmppMucItem::Affiliation>(item->affiliation));
        mucItem.setJid(item->jid);
        permissions << mucItem;
    }

    m_room->setPermissions(permissions);
}

void RoomPermissionModel::_q_permissionsReceived(const QList<QXmppMucItem> &permissions)
{
    foreach (const QXmppMucItem &mucItem, permissions)
        setPermission(mucItem.jid(), mucItem.affiliation());
}

