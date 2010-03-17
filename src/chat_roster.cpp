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

#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppUtils.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_roster.h"
#include "chat_roster_item.h"

enum RosterColumns {
    ContactColumn = 0,
    ImageColumn,
    SortingColumn,
    MaxColumn,
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

ChatRosterPrompt::ChatRosterPrompt(QXmppClient *client, const QString &jid, QWidget *parent)
    : QMessageBox(parent), m_client(client), m_jid(jid)
{
    setText(tr("%1 has asked to add you to his or her contact list.\n\nDo you accept?").arg(jid));
    setWindowModality(Qt::NonModal);
    setWindowTitle(tr("Invitation from %1").arg(jid));

    setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    setDefaultButton(QMessageBox::Yes);
    setEscapeButton(QMessageBox::No);

    connect(this, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotButtonClicked(QAbstractButton*)));
}

void ChatRosterPrompt::slotButtonClicked(QAbstractButton *button)
{
    QXmppPresence packet;
    packet.setTo(m_jid);
    if (standardButton(button) == QMessageBox::Yes)
    {
        qDebug("Subscribe accepted");
        packet.setType(QXmppPresence::Subscribed);
        m_client->sendPacket(packet);

        packet.setType(QXmppPresence::Subscribe);
        m_client->sendPacket(packet);
    } else {
        qDebug("Subscribe refused");
        packet.setType(QXmppPresence::Unsubscribed);
        m_client->sendPacket(packet);
    }
}

ChatRosterModel::ChatRosterModel(QXmppClient *xmppClient)
    : client(xmppClient)
{
    rootItem = new ChatRosterItem(ChatRosterItem::Root);
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(&client->getRoster(), SIGNAL(presenceChanged(const QString&, const QString&)), this, SLOT(presenceChanged(const QString&, const QString&)));
    connect(&client->getRoster(), SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(&client->getRoster(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
    connect(&client->getVCardManager(), SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));
}

ChatRosterModel::~ChatRosterModel()
{
    delete rootItem;
}

int ChatRosterModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

void ChatRosterModel::connected()
{
    /* request own vCard */
    nickName = client->getConfiguration().user();
    client->getVCardManager().requestVCard(
        client->getConfiguration().jidBare());
}

QPixmap ChatRosterModel::contactAvatar(const QString &bareJid) const
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
        return item->data(AvatarRole).value<QPixmap>();
    return QPixmap();
}

QStringList ChatRosterModel::contactFeaturing(const QString &bareJid, ChatRosterModel::Feature feature) const
{
    QStringList jids;
    const QString sought = bareJid + "/";
    foreach (const QString &key, clientFeatures.keys())
        if (key.startsWith(sought) && (clientFeatures.value(key) & feature))
            jids << key;
    return jids;
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
    } else if (role == Qt::DisplayRole && index.column() == ImageColumn) {
        return QVariant();
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
        } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
            return QVariant();
        }
    }
    return item->data(role);
}

void ChatRosterModel::disconnected()
{
    clientFeatures.clear();
    rootItem->clear();
    reset();
}

void ChatRosterModel::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (!clientFeatures.contains(disco.from()) ||
        disco.type() != QXmppIq::Result ||
        disco.queryType() != QXmppDiscoveryIq::InfoQuery)
        return;

    int features = 0;
    foreach (const QString &var, disco.features())
    {
        if (var == ns_chat_states)
            features |= ChatStatesFeature;
        else if (var == ns_stream_initiation_file_transfer)
            features |= FileTransferFeature;
        else if (var == ns_version)
            features |= VersionFeature;
    }
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities())
    {
        if (id.name() == "iChatAgent")
            features |= ChatStatesFeature;
    }
    clientFeatures.insert(disco.from(), features);
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

QString ChatRosterModel::ownName() const
{
    return nickName;
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
    const QString jid = presence.from();
    const QString bareJid = jidToBareJid(jid);
    const QString resource = jidToResource(jid);

    // handle features discovery
    if (!resource.isEmpty())
    {
        if (presence.getType() == QXmppPresence::Unavailable)
            clientFeatures.remove(jid);
        else if (presence.getType() == QXmppPresence::Available && !clientFeatures.contains(jid))
        {
            clientFeatures.insert(jid, 0);

            // discover remote party features
            QXmppDiscoveryIq disco;
            disco.setTo(jid);
            disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
            client->sendPacket(disco);
        }
    }

    // handle chat rooms
    ChatRosterItem *roomItem = rootItem->find(bareJid);
    if (!roomItem || roomItem->type() != ChatRosterItem::Room)
        return;

    ChatRosterItem *memberItem = roomItem->find(jid);
    if (presence.getType() == QXmppPresence::Available && !memberItem)
    {
        beginInsertRows(createIndex(roomItem->row(), 0, roomItem), roomItem->size(), roomItem->size());
        roomItem->append(new ChatRosterItem(ChatRosterItem::RoomMember, jid));
        endInsertRows();

        // check whether we own the room
        foreach (const QXmppElement &x, presence.extensions())
        {
            if (x.tagName() == "x" && x.attribute("xmlns") == ns_muc_user)
            {
                QXmppElement item = x.firstChildElement("item");
                if (item.attribute("jid") == client->getConfiguration().jid())
                {
                    int flags = 0;
                    // role
                    if (item.attribute("role") == "moderator")
                        flags |= KickFlag;
                    // affiliation
                    if (item.attribute("affiliation") == "owner")
                        flags |= (OptionsFlag | MembersFlag);
                    else if (item.attribute("affiliation") == "admin")
                        flags |= MembersFlag;
                    roomItem->setData(FlagsRole, flags);
                }
            }
        }
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
        if (entry.subscriptionType() == QXmppRoster::QXmppRosterEntry::Remove)
        {
            beginRemoveRows(QModelIndex(), item->row(), item->row());
            rootItem->remove(item);
            endRemoveRows();
            return;
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
    client->getVCardManager().requestVCard(jid);
}

void ChatRosterModel::rosterReceived()
{
    // remove existing contacts
    QList <ChatRosterItem*> goners;
    for (int i = 0; i < rootItem->size(); i++)
    {
        ChatRosterItem *child = rootItem->child(i);
        if (child->type() == ChatRosterItem::Contact)
            goners << child;
    }
    foreach (ChatRosterItem *child, goners)
        rootItem->remove(child);

    // add received contacts
    foreach (const QString &jid, client->getRoster().getRosterBareJids())
    {
        rootItem->append(new ChatRosterItem(ChatRosterItem::Contact, jid));

        // fetch vCard
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
    const QString bareJid = vcard.from();
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        const QImage &image = vcard.photoAsImage();
        item->setData(AvatarRole, QPixmap::fromImage(image));
        if (!vcard.nickName().isEmpty())
            item->setData(Qt::DisplayRole, vcard.nickName());
        else if (!vcard.fullName().isEmpty())
            item->setData(Qt::DisplayRole, vcard.fullName());

        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
    if (bareJid == client->getConfiguration().jidBare())
    {
        if (!vcard.nickName().isEmpty())
            nickName = vcard.nickName();
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

void ChatRosterModel::addItem(ChatRosterItem::Type type, const QString &id, const QString &name, const QIcon &icon)
{
    if (rootItem->find(id))
        return;

    // prepare item
    ChatRosterItem *item = new ChatRosterItem(type, id);
    if (!name.isEmpty())
        item->setData(Qt::DisplayRole, name);
    if (!icon.isNull())
        item->setData(Qt::DecorationRole, icon);

    // add item
    beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
    rootItem->append(item);
    endInsertRows();
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

void ChatRosterModel::removeItem(const QString &bareJid)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        beginRemoveRows(QModelIndex(), item->row(), item->row());
        rootItem->remove(item);
        endRemoveRows();
    }
}

ChatRosterView::ChatRosterView(ChatRosterModel *model, QWidget *parent)
    : QTreeView(parent), rosterModel(model)
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

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString &bareJid = index.data(ChatRosterModel::IdRole).toString();
    
    if (type == ChatRosterItem::Contact)
    {
        QMenu *menu = new QMenu(this);

        QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Invite to a chat room"));
        action->setData(InviteAction);
        connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));

        if (!rosterModel->contactFeaturing(bareJid, ChatRosterModel::FileTransferFeature).isEmpty())
        {
            action = menu->addAction(QIcon(":/add.png"), tr("Send a file"));
            action->setData(SendAction);
            connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));
        }

#if 0
        if (!rosterModel->contactFeaturing(bareJid, ChatRosterModel::VersionFeature).isEmpty())
        {
            action = menu->addAction(QIcon(":/diagnostics.png"), tr("Information"));
            action->setData(OptionsAction);
            connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));
        }
#endif

        action = menu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
        action->setData(RemoveAction);
        connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));

        menu->popup(event->globalPos());
    } else if (type == ChatRosterItem::Room) {
        int flags = index.data(ChatRosterModel::FlagsRole).toInt();
        if (flags & ChatRosterModel::OptionsFlag || flags & ChatRosterModel::MembersFlag)
        {
            QMenu *menu = new QMenu(this);

            if (flags & ChatRosterModel::OptionsFlag)
            {
                QAction *action = menu->addAction(QIcon(":/options.png"), tr("Options"));
                action->setData(OptionsAction);
                connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));
            }

            if (flags & ChatRosterModel::MembersFlag)
            {
                QAction *action = menu->addAction(QIcon(":/chat.png"), tr("Members"));
                action->setData(MembersAction);
                connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));
            }

            menu->popup(event->globalPos());
        }
    } else if (type == ChatRosterItem::RoomMember) {
        QModelIndex room = index.parent();
        if (room.data(ChatRosterModel::FlagsRole).toInt() & ChatRosterModel::KickFlag)
        {
            QMenu *menu = new QMenu(this);

            QAction *action = menu->addAction(QIcon(":/remove.png"), tr("Kick user"));
            action->setData(RemoveAction);
            connect(action, SIGNAL(triggered()), this, SLOT(slotAction()));

            menu->popup(event->globalPos());
        }
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
        if (index.data(ChatRosterModel::IdRole).toString() == jid)
        {
            if (index != currentIndex())
                setCurrentIndex(index);
            return;
        }
    }
    setCurrentIndex(QModelIndex());
}

void ChatRosterView::selectionChanged(const QItemSelection & selected, const QItemSelection &deselected)
{
    foreach (const QModelIndex &index, selected.indexes())
        expand(index);
}

QSize ChatRosterView::sizeHint () const
{
    if (!model()->rowCount())
        return QTreeView::sizeHint();

    QSize hint(200, 0);
    hint.setHeight(model()->rowCount() * sizeHintForRow(0));
    return hint;
}

void ChatRosterView::slotAction()
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    emit itemAction(action->data().toInt(),
        index.data(ChatRosterModel::IdRole).toString(),
        index.data(ChatRosterModel::TypeRole).toInt());
}

void ChatRosterView::slotActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    emit itemAction(JoinAction,
        index.data(ChatRosterModel::IdRole).toString(),
        index.data(ChatRosterModel::TypeRole).toInt());
}

