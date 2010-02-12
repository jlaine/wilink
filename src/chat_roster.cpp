/*
 * wDesktop
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

#include <QContextMenuEvent>
#include <QDebug>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QStringList>
#include <QSortFilterProxyModel>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_roster.h"
#include "chat_roster_item.h"

enum RosterColumns {
    ContactColumn = 0,
    ImageColumn,
    SortingColumn,
    MaxColumn,
};

enum RosterRoles {
    IdRole = Qt::UserRole,
    TypeRole,
    MessagesRole,
};

static void paintMessages(QPixmap &icon, int messages)
{
    QString pending = QString::number(messages);
    QPainter painter(&icon);
    QFont font = painter.font();
    font.setWeight(QFont::Bold);
    painter.setFont(font);

    // text rectangle
    QRect rect = painter.fontMetrics().boundingRect(pending);
    rect.setWidth(rect.width() + 4);
    if (rect.width() < rect.height())
        rect.setWidth(rect.height());
    else
        rect.setHeight(rect.width());
    rect.moveTop(2);
    rect.moveRight(icon.width() - 2);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.drawEllipse(rect);
    painter.drawText(rect, Qt::AlignCenter, pending);
}

ChatRosterModel::ChatRosterModel(QXmppClient *xmppClient)
    : client(xmppClient)
{
    rootItem = new ChatRosterItem(ChatRosterItem::Root);
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(&client->getRoster(), SIGNAL(presenceChanged(const QString&, const QString&)), this, SLOT(presenceChanged(const QString&, const QString&)));
    connect(&client->getRoster(), SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(&client->getRoster(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
    connect(&client->getVCardManager(), SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));
}

int ChatRosterModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QPixmap ChatRosterModel::contactAvatar(const QString &bareJid) const
{
    if (rosterAvatars.contains(bareJid))
        return rosterAvatars[bareJid];
    return QPixmap();
}

QString ChatRosterModel::contactName(const QString &bareJid) const
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
        return item->data(Qt::DisplayRole).toString();
    return bareJid.split("@").first();
}

QString ChatRosterModel::contactStatus(const QString &bareJid) const
{
    QString suffix = "offline";
    foreach (const QXmppPresence &presence, client->getRoster().getAllPresencesForBareJid(bareJid))
    {
        if (presence.getType() != QXmppPresence::Available)
            continue;
        if (presence.getStatus().getType() == QXmppPresence::Status::Online)
        {
            suffix = "available";
            break;
        } else {
            suffix = "busy";
        }
    }
    return suffix;
}

QVariant ChatRosterModel::data(const QModelIndex &index, int role) const
{
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    QString bareJid = item->id();
    int messages = item->data(MessagesRole).toInt();

    if (role == IdRole) {
        return bareJid;
    } else if (role == TypeRole) {
        return item->type();
    } else if (role == Qt::DisplayRole && index.column() == ContactColumn) {
        return item->data(Qt::DisplayRole).toString();
    } else if(role == Qt::FontRole && index.column() == ContactColumn) {
        if (messages)
            return QFont("", -1, QFont::Bold, true);
    } else if(role == Qt::BackgroundRole && index.column() == ContactColumn) {
        if (messages)
        {
            QLinearGradient grad(QPointF(0, 0), QPointF(0.8, 0));
            grad.setColorAt(0, QColor(255, 0, 0, 144));
            grad.setColorAt(1, Qt::transparent);
            grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            return QBrush(grad);
        }
    } else {
        if (item->type() == ChatRosterItem::Contact)
        {
            const QXmppRoster::QXmppRosterEntry &entry = client->getRoster().getRosterEntry(bareJid);
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap icon(QString(":/contact-%1.png").arg(contactStatus(bareJid)));
                if (messages)
                    paintMessages(icon, messages);
                return icon;
            } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
                return QIcon(contactAvatar(bareJid));
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return (contactStatus(bareJid) + "_" + contactName(bareJid)).toLower() + "_" + bareJid.toLower();
            }
        } else if (item->type() == ChatRosterItem::Room) {
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap icon(":/chat.png");
                if (messages)
                    paintMessages(icon, messages);
                return icon;
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return QString("chatroom_") + bareJid.toLower();
            }
        } else if (item->type() == ChatRosterItem::RoomMember) {
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                return QIcon(":/contact-available.png");
            }
            if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return QString("chatuser_") + bareJid.toLower();
            }
        }
    }
    return QVariant();
}

void ChatRosterModel::disconnected()
{
    rootItem->clear();
    reset();
}

QModelIndex ChatRosterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());

    ChatRosterItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ChatRosterModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    ChatRosterItem *childItem = static_cast<ChatRosterItem*>(index.internalPointer());
    ChatRosterItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

void ChatRosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
}

void ChatRosterModel::presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.getFrom();
    const QString roomJid = jid.split("/")[0];
    ChatRosterItem *roomItem = rootItem->find(roomJid);
    if (!roomItem || roomItem->type() != ChatRosterItem::Room)
        return;

    ChatRosterItem *memberItem = roomItem->find(jid);
    if (presence.getType() == QXmppPresence::Available && !memberItem)
    {
        beginInsertRows(createIndex(roomItem->row(), 0, roomItem), roomItem->size(), roomItem->size());
        roomItem->append(new ChatRosterItem(ChatRosterItem::RoomMember, jid));
        endInsertRows();
    }
    else if (presence.getType() == QXmppPresence::Unavailable && memberItem)
    {
        beginRemoveRows(createIndex(roomItem->row(), 0, roomItem), memberItem->row(), memberItem->row());
        roomItem->remove(memberItem);
        endRemoveRows();
    }
}

void ChatRosterModel::rosterChanged(const QString &jid)
{
    ChatRosterItem *item = rootItem->find(jid);
    if (item)
    {
        QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(jid);
        if (static_cast<int>(entry.getSubscriptionType()) == static_cast<int>(QXmppRosterIq::Item::Remove))
        {
            beginRemoveRows(QModelIndex(), item->row(), item->row());
            rootItem->remove(item);
            endRemoveRows();
        } else {
            emit dataChanged(createIndex(item->row(), ContactColumn, item),
                             createIndex(item->row(), SortingColumn, item));
        }
    } else {
        beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
        rootItem->append(new ChatRosterItem(ChatRosterItem::Contact, jid));
        endInsertRows();
    }

    // fetch vCard
    if (!rosterAvatars.contains(jid))
        client->getVCardManager().requestVCard(jid);
}

void ChatRosterModel::rosterReceived()
{
    rootItem->clear();
    foreach (const QString &jid, client->getRoster().getRosterBareJids())
    {
        rootItem->append(new ChatRosterItem(ChatRosterItem::Contact, jid));

        // fetch vCard
        if (!rosterAvatars.contains(jid))
            client->getVCardManager().requestVCard(jid);
    }
    reset();
}

int ChatRosterModel::rowCount(const QModelIndex &parent) const
{
    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());
    return parentItem->size();
}

void ChatRosterModel::vCardReceived(const QXmppVCard& vcard)
{
    const QString bareJid = vcard.getFrom();
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        const QImage &image = vcard.getPhotoAsImage();
        rosterAvatars[bareJid] = QPixmap::fromImage(image);
        if (!vcard.getNickName().isEmpty())
            item->setData(Qt::DisplayRole, vcard.getNickName());

        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

void ChatRosterModel::addPendingMessage(const QString &bareJid)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        item->setData(MessagesRole, item->data(MessagesRole).toInt() + 1);
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

void ChatRosterModel::addRoom(const QString &bareJid)
{
    if (rootItem->find(bareJid))
        return;
    beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
    rootItem->append(new ChatRosterItem(ChatRosterItem::Room, bareJid));
    endInsertRows();

#if 0
    // discover room info
    QXmppDiscoveryIq disco;
    disco.setTo(bareJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    xmppClient->sendPacket(disco);
#endif
}

void ChatRosterModel::clearPendingMessages(const QString &bareJid)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        item->setData(MessagesRole, 0);
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

void ChatRosterModel::removeRoom(const QString &bareJid)
{
    ChatRosterItem *roomItem = rootItem->find(bareJid);
    if (roomItem)
    {
        beginRemoveRows(QModelIndex(), roomItem->row(), roomItem->row());
        rootItem->remove(roomItem);
        endRemoveRows();
    }
}

ChatRosterView::ChatRosterView(ChatRosterModel *model, QWidget *parent)
    : QTreeView(parent)
{
    QSortFilterProxyModel *sortedModel = new QSortFilterProxyModel(this);
    sortedModel->setSourceModel(model);
    sortedModel->setDynamicSortFilter(true);
    setModel(sortedModel);

    /* prepare context menu */
    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotActivated(const QModelIndex&)));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotActivated(const QModelIndex&)));

    setAlternatingRowColors(true);
    setColumnHidden(SortingColumn, true);
    setColumnWidth(ImageColumn, 40);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setHeaderHidden(true);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(SortingColumn, Qt::AscendingOrder);
}

void ChatRosterView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    int type = index.data(TypeRole).toInt();
    if (type == ChatRosterItem::Contact)
    {
        QMenu *menu = new QMenu(this);
        QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Invite to a chat room"));
        connect(action, SIGNAL(triggered()), this, SLOT(slotInviteContact()));
        action = menu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
        connect(action, SIGNAL(triggered()), this, SLOT(slotRemoveContact()));
        menu->popup(event->globalPos());
    } else if (type == ChatRosterItem::Room) {
        QMenu *menu = new QMenu(this);
        QAction *action = menu->addAction(QIcon(":/remove.png"), tr("Leave room"));
        connect(action, SIGNAL(triggered()), this, SLOT(slotLeaveRoom()));
        menu->popup(event->globalPos());
    }
}

void ChatRosterView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    setColumnWidth(ContactColumn, e->size().width() - 40);
}

void ChatRosterView::selectContact(const QString &jid)
{
    for (int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index = model()->index(i, 0);
        if (index.data(IdRole).toString() == jid)
        {
            setCurrentIndex(index);
            return;
        }
    }
    setCurrentIndex(QModelIndex());
}

QSize ChatRosterView::sizeHint () const
{
    if (!model()->rowCount())
        return QTreeView::sizeHint();

    QSize hint(200, 0);
    hint.setHeight(model()->rowCount() * sizeHintForRow(0));
    return hint;
}

void ChatRosterView::slotActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const int type = index.data(TypeRole).toInt();
    const QString jid = index.data(IdRole).toString();
    if (type == ChatRosterItem::Contact)
        emit joinConversation(jid, false);
    else if (type == ChatRosterItem::Room)
        emit joinConversation(jid, true);
}

void ChatRosterView::slotLeaveRoom()
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    const int type = index.data(TypeRole).toInt();
    if (type == ChatRosterItem::Room)
    {
        const QString jid = index.data(Qt::UserRole).toString().split("/")[0];
        emit leaveConversation(jid, true);
    }
}

void ChatRosterView::slotInviteContact()
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    const int type = index.data(TypeRole).toInt();
    if (type == ChatRosterItem::Contact)
        emit inviteContact(index.data(IdRole).toString());
}

void ChatRosterView::slotRemoveContact()
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    const int type = index.data(TypeRole).toInt();
    if (type == ChatRosterItem::Contact)
        emit removeContact(index.data(IdRole).toString());
}

