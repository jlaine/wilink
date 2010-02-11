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
#include <QHeaderView>
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

RosterModel::RosterModel(QXmppRoster *roster, QXmppVCardManager *vcard)
    : rosterManager(roster), vcardManager(vcard)
{
    rootItem = new ChatRosterItem("");
    connect(rosterManager, SIGNAL(presenceChanged(const QString&, const QString&)), this, SLOT(presenceChanged(const QString&, const QString&)));
    connect(rosterManager, SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(rosterManager, SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
    connect(vcardManager, SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));
}

int RosterModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QPixmap RosterModel::contactAvatar(const QString &bareJid) const
{
    if (rosterAvatars.contains(bareJid))
        return rosterAvatars[bareJid];
    return QPixmap();
}

QString RosterModel::contactName(const QString &bareJid) const
{
    if (rosterNames.contains(bareJid) && !rosterNames[bareJid].isEmpty())
        return rosterNames[bareJid];
    return bareJid.split("@").first();
}

QString RosterModel::contactStatus(const QString &bareJid) const
{
    QMap<QString, QXmppPresence> presences = rosterManager->getAllPresencesForBareJid(bareJid);
    if(presences.isEmpty())
        return "offline";
    QXmppPresence presence = presences[contactName(bareJid)];
    if (presence.getType() != QXmppPresence::Available)
        return "offline";
    if (presence.getStatus().getType() == QXmppPresence::Status::Online)
        return "available";

    return "busy";
}

QPixmap RosterModel::contactStatusIcon(const QString &bareJid) const
{
    QPixmap icon(QString(":/contact-%1.png").arg(contactStatus(bareJid)));

    if (pendingMessages.contains(bareJid))
    {
        QString pending = QString::number(pendingMessages[bareJid]);
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

    return icon;
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    QString bareJid = item->id();
    const QXmppRoster::QXmppRosterEntry &entry = rosterManager->getRosterEntry(bareJid);
    if (role == Qt::UserRole) {
        return bareJid;
    } else if (role == Qt::DisplayRole && index.column() == ContactColumn) {
        return contactName(bareJid);
    } else if (role == Qt::DecorationRole && index.column() == ContactColumn) {
        return QIcon(contactStatusIcon(entry.getBareJid()));
    } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
        return QIcon(contactAvatar(bareJid));
    } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
        return (contactStatus(bareJid) + "_" + contactName(bareJid)).toLower() + "_" + bareJid.toLower();
    } else if(role == Qt::FontRole && index.column() == ContactColumn) {
        if (pendingMessages.contains(bareJid))
            return QFont("", -1, QFont::Bold, true);
    } else if(role == Qt::BackgroundRole && index.column() == ContactColumn) {
        if (pendingMessages.contains(bareJid)) {
            QLinearGradient grad(QPointF(0, 0), QPointF(0.8, 0));
            grad.setColorAt(0, QColor(255, 0, 0, 144));
            grad.setColorAt(1, Qt::transparent);
            grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            return QBrush(grad);
        }
    }
    return QVariant();
}

void RosterModel::disconnected()
{
    rootItem->clear();
    reset();
}

QModelIndex RosterModel::index(int row, int column, const QModelIndex &parent) const
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

void RosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
}

void RosterModel::rosterChanged(const QString &jid)
{
    QXmppRoster::QXmppRosterEntry entry = rosterManager->getRosterEntry(jid);
    ChatRosterItem *item = rootItem->find(jid);
    if (item)
    {
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
        rootItem->append(new ChatRosterItem(jid));
        endInsertRows();
    }

    // fetch vCard
    if (!rosterAvatars.contains(jid))
        vcardManager->requestVCard(jid);
}

void RosterModel::rosterReceived()
{
    rootItem->clear();
    foreach (const QString &jid, rosterManager->getRosterBareJids())
    {
        rootItem->append(new ChatRosterItem(jid));

        // fetch vCard
        if (!rosterAvatars.contains(jid))
            vcardManager->requestVCard(jid);
    }
    reset();
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());
    return parentItem->size();
}

void RosterModel::vCardReceived(const QXmppVCard& vcard)
{
    const QString bareJid = vcard.getFrom();
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        const QImage &image = vcard.getPhotoAsImage();
        rosterAvatars[bareJid] = QPixmap::fromImage(image);
        rosterNames[bareJid] = vcard.getNickName();

        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

void RosterModel::addPendingMessage(const QString &bareJid)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        if (pendingMessages.contains(bareJid))
            pendingMessages[bareJid]++;
        else
            pendingMessages[bareJid] = 1;

        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

void RosterModel::clearPendingMessages(const QString &bareJid)
{
    ChatRosterItem *item = rootItem->find(bareJid);
    if (item)
    {
        pendingMessages.remove(bareJid);
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
}

RosterView::RosterView(RosterModel *model, QWidget *parent)
    : QTableView(parent)
{
    QSortFilterProxyModel *sortedModel = new QSortFilterProxyModel(this);
    sortedModel->setSourceModel(model);
    sortedModel->setDynamicSortFilter(true);
    setModel(sortedModel);

    /* prepare context menu */
    QAction *action;
    contextMenu = new QMenu(this);
    action = contextMenu->addAction(QIcon(":/chat.png"), tr("Start chat"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotDoubleClicked()));
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(slotRemoveContact()));

    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotClicked()));
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClicked()));

    setAlternatingRowColors(true);
    setColumnHidden(SortingColumn, true);
    setColumnWidth(ImageColumn, 40);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setShowGrid(false);
    setSortingEnabled(true);
    sortByColumn(SortingColumn, Qt::AscendingOrder);
    horizontalHeader()->setResizeMode(ContactColumn, QHeaderView::Stretch);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
}

void RosterView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    contextMenu->popup(event->globalPos());
}

void RosterView::selectContact(const QString &jid)
{
    for (int i = 0; i < model()->rowCount(); i++)
    {
        QModelIndex index = model()->index(i, 0);
        if (index.data(Qt::UserRole).toString() == jid)
        {
            setCurrentIndex(index);
            return;
        }
    }
    setCurrentIndex(QModelIndex());
}

QSize RosterView::sizeHint () const
{
    if (!model()->rowCount())
        return QTableView::sizeHint();

    QSize hint(64, 0);
    hint.setHeight(model()->rowCount() * sizeHintForRow(0));
    for (int i = 0; i < SortingColumn; i++)
        hint.setWidth(hint.width() + sizeHintForColumn(i));
    return hint;
}

void RosterView::slotClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit clicked(index.data(Qt::UserRole).toString());
}

void RosterView::slotDoubleClicked()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit doubleClicked(index.data(Qt::UserRole).toString());
}

void RosterView::slotRemoveContact()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit removeContact(index.data(Qt::UserRole).toString());
}

