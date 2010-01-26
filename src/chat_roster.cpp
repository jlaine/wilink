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
#include <QStringList>
#include <QSortFilterProxyModel>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_roster.h"

enum RosterColumns {
    ContactColumn = 0,
    ImageColumn,
    SortingColumn,
    MaxColumn,
};

RosterModel::RosterModel(QXmppRoster *roster, QXmppVCardManager *vcard)
    : rosterManager(roster), vcardManager(vcard)
{
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

QString RosterModel::contactStatusIcon(const QString &bareJid) const
{
    if(pendingMessages.contains(bareJid))
        return QString(":/contact-aPendingMessage.png");
    return QString(":/contact-%1.png").arg(contactStatus(bareJid));
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rosterKeys.size())
        return QVariant();

    const QXmppRoster::QXmppRosterEntry &entry = rosterManager->getRosterEntry(rosterKeys.at(index.row()));
    const QString bareJid = entry.getBareJid();
    if (role == Qt::UserRole) {
        return bareJid;
    } else if (role == Qt::DisplayRole && index.column() == ContactColumn) {
        return contactName(bareJid);
    } else if (role == Qt::DecorationRole && index.column() == ContactColumn) {
        return QIcon(contactStatusIcon(entry.getBareJid()));
    } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
        if (rosterAvatars.contains(bareJid))
            return QIcon(rosterAvatars[bareJid]);
    } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
        return (contactStatus(bareJid) + "_" + contactName(bareJid)).toLower();
    }
    return QVariant();
}

void RosterModel::disconnected()
{
    rosterKeys.clear();
    reset();
}

void RosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    const int rowIndex = rosterKeys.indexOf(bareJid);
    if (rowIndex >= 0)
        emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, SortingColumn));
}

void RosterModel::rosterChanged(const QString &jid)
{
    QXmppRoster::QXmppRosterEntry entry = rosterManager->getRosterEntry(jid);
    const int rowIndex = rosterKeys.indexOf(jid);
    if (rowIndex >= 0)
    {
        if (static_cast<int>(entry.getSubscriptionType()) == static_cast<int>(QXmppRosterIq::Item::Remove))
        {
            beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
            rosterKeys.removeAt(rowIndex);
            endRemoveRows();
        } else {
            emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, SortingColumn));
        }
    } else {
        beginInsertRows(QModelIndex(), rosterKeys.length(), rosterKeys.length());
        rosterKeys.append(jid);
        endInsertRows();
    }
}

void RosterModel::rosterReceived()
{
    rosterKeys = rosterManager->getRosterBareJids();
    foreach (const QString &jid, rosterKeys)
    {
        if (!rosterAvatars.contains(jid))
            vcardManager->requestVCard(jid);
    }
    reset();
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    return rosterKeys.size();
}

void RosterModel::vCardReceived(const QXmppVCard& vcard)
{
    const QString bareJid = vcard.getFrom();
    const int rowIndex = rosterKeys.indexOf(bareJid);
    if (rowIndex >= 0)
    {
        const QImage &image = vcard.getPhotoAsImage();
        rosterAvatars[bareJid] = QPixmap::fromImage(image);
        const QString &nickname = vcard.getNickName();
        rosterNames[bareJid] = vcard.getNickName();
        emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, SortingColumn));
    }
}

void RosterModel::removePendingMessage(const QString &bareJid)
{
    pendingMessages.remove(bareJid);
    const int rowIndex = rosterKeys.indexOf(bareJid);
    emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, SortingColumn));
}

void RosterModel::setPendingMessage(const QString &bareJid)
{
    if (pendingMessages.contains(bareJid))
        pendingMessages[bareJid]++;
    else
        pendingMessages[bareJid] = 1;
    const int rowIndex = rosterKeys.indexOf(bareJid);
    emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, SortingColumn));
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
    connect(action, SIGNAL(triggered()), this, SLOT(startChat()));
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

#ifdef CHAT_SINGLEWINDOW
    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(startChat()));
#else
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(startChat()));
#endif

    setAlternatingRowColors(true);
    setColumnHidden(SortingColumn, true);
    setColumnWidth(ImageColumn, 40);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setIconSize(QSize(32, 32));
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

void RosterView::removeContact()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit removeContact(index.data(Qt::UserRole).toString());
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

void RosterView::startChat()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit chatContact(index.data(Qt::UserRole).toString());
}

