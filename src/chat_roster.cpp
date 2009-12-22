/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_roster.h"

enum RosterColumns {
    ContactColumn = 0,
    ImageColumn,
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

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rosterKeys.size())
        return QVariant();

    const QXmppRoster::QXmppRosterEntry &entry = rosterManager->getRosterEntry(rosterKeys.at(index.row()));
    const QString bareJid = entry.getBareJid();
    if (role == Qt::UserRole) {
        return bareJid;
    } else if (role == Qt::DisplayRole && index.column() == ContactColumn) {
        QString name = entry.getName();
        if (name.isEmpty())
            name = bareJid.split("@").first();
        return name;
    } else if (role == Qt::DecorationRole && index.column() == ContactColumn) {
        QString suffix = "offline";
        foreach (const QXmppPresence &presence, rosterManager->getAllPresencesForBareJid(entry.getBareJid()))
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
        return QIcon(QString(":/contact-%1.png").arg(suffix));
    } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
        if (rosterIcons.contains(bareJid))
            return rosterIcons[bareJid];
    }
    return QVariant();
}

void RosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    const int rowIndex = rosterKeys.indexOf(bareJid);
    if (rowIndex >= 0)
        emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, ContactColumn));
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
            emit dataChanged(index(rowIndex, ContactColumn), index(rowIndex, ContactColumn));
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
        if (!rosterIcons.contains(jid))
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
        rosterIcons[bareJid] = QIcon(QPixmap::fromImage(image));
        emit dataChanged(index(rowIndex, ImageColumn), index(rowIndex, ImageColumn));
    }
}

RosterView::RosterView(QXmppClient &client, QWidget *parent)
    : QTableView(parent)
{
    setModel(new RosterModel(&client.getRoster(), &client.getVCardManager()));

    /* prepare context menu */
    QAction *action;
    contextMenu = new QMenu(this);
    action = contextMenu->addAction(QIcon(":/chat.png"), tr("Start chat"));
    connect(action, SIGNAL(triggered()), this, SLOT(startChat()));
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(startChat()));

    setAlternatingRowColors(true);
    setColumnWidth(ImageColumn, 40);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setIconSize(QSize(32, 32));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setShowGrid(false);
    horizontalHeader()->setResizeMode(ContactColumn, QHeaderView::Stretch);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);

    setMinimumSize(QSize(140, 140));
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

void RosterView::startChat()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit chatContact(index.data(Qt::UserRole).toString());
}

