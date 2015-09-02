/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#include <QUrl>

#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"

#include "client.h"
#include "history.h"
#include "rooms.h"
#include "roster.h"

RoomConfigurationModel::RoomConfigurationModel(QObject *parent)
    : QAbstractListModel(parent),
    m_room(0)
{
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

QHash<int, QByteArray> RoomConfigurationModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(DescriptionRole, "description");
    roleNames.insert(KeyRole, "key");
    roleNames.insert(LabelRole, "label");
    roleNames.insert(TypeRole, "type");
    roleNames.insert(ValueRole, "value");
    return roleNames;
}

QXmppMucRoom *RoomConfigurationModel::room() const
{
    return m_room;
}

void RoomConfigurationModel::setRoom(QXmppMucRoom *room)
{
    bool check;
    Q_UNUSED(check);

    if (room != m_room) {
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

void RoomConfigurationModel::setValue(int row, const QVariant &value)
{
    if (row > 0 && row <  m_form.fields().size() &&
        value != m_form.fields()[row].value()) {
        m_form.fields()[row].setValue(value);
        emit dataChanged(index(row), index(row));
    }
}

bool RoomConfigurationModel::submit()
{
    if (m_room) {
        return m_room->setConfiguration(m_form);
    } else {
        return false;
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

class ChatRoomItem : public ChatModelItem
{
public:
    QString jid;
    QXmppPresence::AvailableStatusType status;
    QXmppMucItem::Affiliation affiliation;
};

RoomModel::RoomModel(QObject *parent)
    : ChatModel(parent)
    , m_manager(0)
    , m_room(0)
{
    m_historyModel = new HistoryModel(this);

    connect(VCardCache::instance(), SIGNAL(cardChanged(QString)),
            this, SLOT(_q_participantChanged(QString)));
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
        return QXmppUtils::jidToResource(item->jid);
    } else if (role == RoomModel::AffiliationRole) {
        return item->affiliation;
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

QHash<int, QByteArray> RoomModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ChatModel::AvatarRole, "avatar");
    roleNames.insert(ChatModel::JidRole, "jid");
    roleNames.insert(ChatModel::NameRole, "name");
    roleNames.insert(RoomModel::AffiliationRole, "affiliation");
    return roleNames;
}

void RoomModel::_q_messageReceived(const QXmppMessage &msg)
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
        ChatClient *client = m_manager ? qobject_cast<ChatClient*>(m_manager->parent()) : 0;
        if (client)
            message.date = client->serverTime();
        else
            message.date = QDateTime::currentDateTime();
    }
    message.jid = msg.from();
    message.received = QXmppUtils::jidToResource(msg.from()) != m_room->nickName();
    m_historyModel->addMessage(message);
}

void RoomModel::_q_participantAdded(const QString &jid)
{
    Q_ASSERT(m_room);
    //qDebug("participant added %s", qPrintable(jid));

    const QXmppMucItem::Affiliation affiliation = m_room->participantPresence(jid).mucItem().affiliation();
    int row = rootItem->children.size();
    foreach (ChatModelItem *ptr, rootItem->children) {
        ChatRoomItem *item = static_cast<ChatRoomItem*>(ptr);
        if (item->jid == jid) {
            qWarning("participant added twice %s", qPrintable(jid));
            return;
        } else if (affiliation > item->affiliation ||
                   (affiliation == item->affiliation && item->jid.compare(jid, Qt::CaseInsensitive) > 0)) {
            row = item->row();
            break;
        }
    }

    ChatRoomItem *item = new ChatRoomItem;
    item->jid = jid;
    item->affiliation = affiliation;
    addItem(item, rootItem, row);
}

void RoomModel::_q_participantChanged(const QString &jid)
{
    Q_ASSERT(m_room);
    //qDebug("participant changed %s", qPrintable(jid));

    foreach (ChatModelItem *ptr, rootItem->children) {
        ChatRoomItem *item = static_cast<ChatRoomItem*>(ptr);
        if (item->jid == jid) {
            if (item->affiliation != m_room->participantPresence(jid).mucItem().affiliation()) {
                removeRow(item->row());
                _q_participantAdded(jid);
            } else {
                item->status = m_room->participantPresence(jid).availableStatusType();
                emit dataChanged(createIndex(item), createIndex(item));
            }
            break;
        }
    }
}

void RoomModel::_q_participantRemoved(const QString &jid)
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
    Q_UNUSED(check);

    if (room == m_room)
        return;

    // disconnect signals
    if (m_room)
        m_room->disconnect(this);

    m_room = room;

    // connect signals
    if (m_room) {
        check = connect(m_room, SIGNAL(messageReceived(QXmppMessage)),
                        this, SLOT(_q_messageReceived(QXmppMessage)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantAdded(QString)),
                        this, SLOT(_q_participantAdded(QString)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantChanged(QString)),
                        this, SLOT(_q_participantChanged(QString)));
        Q_ASSERT(check);

        check = connect(m_room, SIGNAL(participantRemoved(QString)),
                        this, SLOT(_q_participantRemoved(QString)));
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

QHash<int, QByteArray> RoomPermissionModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(AffiliationRole, "affiliation");
    roleNames.insert(JidRole, "jid");
    return roleNames;
}

QXmppMucRoom *RoomPermissionModel::room() const
{
    return m_room;
}

void RoomPermissionModel::setRoom(QXmppMucRoom *room)
{
    bool check;
    Q_UNUSED(check);

    if (room != m_room) {
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

bool RoomPermissionModel::submit()
{
    if (!m_room)
        return false;

    QList<QXmppMucItem> permissions;
    foreach (ChatModelItem *ptr, rootItem->children) {
        RoomPermissionItem *item = static_cast<RoomPermissionItem*>(ptr);

        QXmppMucItem mucItem;
        mucItem.setAffiliation(static_cast<QXmppMucItem::Affiliation>(item->affiliation));
        mucItem.setJid(item->jid);
        permissions << mucItem;
    }

    return m_room->setPermissions(permissions);
}

void RoomPermissionModel::_q_permissionsReceived(const QList<QXmppMucItem> &permissions)
{
    foreach (const QXmppMucItem &mucItem, permissions)
        setPermission(mucItem.jid(), mucItem.affiliation());
}

