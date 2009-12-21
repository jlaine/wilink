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
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_contacts.h"

RosterModel::RosterModel(QXmppRoster *roster)
    : modelRoster(roster)
{
    connect(modelRoster, SIGNAL(presenceChanged(const QString&, const QString&)), this, SLOT(presenceChanged(const QString&, const QString&)));
    connect(modelRoster, SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(modelRoster, SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
    //connect(vcardManager, SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rosterKeys.size())
        return QVariant();

    const QXmppRoster::QXmppRosterEntry &entry = modelRoster->getRosterEntry(rosterKeys.at(index.row()));
    if (role == Qt::DisplayRole)
    {
        QString name = entry.getName();
        if (name.isEmpty())
            name = entry.getBareJid().split("@").first();
        return name;
    } else if (role == Qt::DecorationRole) {
        QString suffix = "offline";
        foreach (const QXmppPresence &presence, modelRoster->getAllPresencesForBareJid(entry.getBareJid()))
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
    } else if (role == BareJidRole) {
        return entry.getBareJid();
    }
    return QVariant();
}

void RosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    const int rowIndex = rosterKeys.indexOf(bareJid);
    if (rowIndex >= 0)
        emit dataChanged(index(rowIndex, Qt::DisplayRole), index(rowIndex, BareJidRole));
}

void RosterModel::rosterChanged(const QString &jid)
{
    QXmppRoster::QXmppRosterEntry entry = modelRoster->getRosterEntry(jid);
    const int rowIndex = rosterKeys.indexOf(jid);
    if (rowIndex >= 0)
    {
        if (entry.getSubscriptionType() == QXmppRosterIq::Item::Remove)
        {
            beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
            rosterKeys.removeAt(rowIndex);
            endRemoveRows();
        } else {
            emit dataChanged(index(rowIndex, Qt::DisplayRole), index(rowIndex, BareJidRole));
        }
    } else {
        beginInsertRows(QModelIndex(), rosterKeys.length(), rosterKeys.length());
        rosterKeys.append(jid);
        endInsertRows();
    }
}

void RosterModel::rosterReceived()
{
    rosterKeys = modelRoster->getRosterBareJids();
    reset();
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    return rosterKeys.size();
}

void RosterModel::vCardReceived(const QXmppVCard& vcard)
{
#ifdef LEGACY_CONTACTS
    QImage image = vcard.getPhotoAsImage();
    const QString bareJid = vcard.getFrom().split("/").first();
    for (int i = 0; i < count(); i++)
    {
        QListWidgetItem *entry = item(i);
        if (entry->data(BareJidRole).toString() == bareJid)
        {
            entry->setIcon(QIcon(QPixmap::fromImage(image)));
            if(!vcard.getFullName().isEmpty())
                entry->setText(vcard.getFullName());
            break;
        }
    }
#endif
}

RosterView::RosterView(QXmppClient &client, QWidget *parent)
    : QListView(parent)
{
    setModel(new RosterModel(&client.getRoster()));

    /* prepare context menu */
    QAction *action;
    contextMenu = new QMenu(this);
    action = contextMenu->addAction(QIcon(":/chat.png"), tr("Start chat"));
    connect(action, SIGNAL(triggered()), this, SLOT(startChat()));
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(startChat()));

    setContextMenuPolicy(Qt::DefaultContextMenu);
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
        emit removeContact(index.data(RosterModel::BareJidRole).toString());
}

void RosterView::startChat()
{
    const QModelIndex &index = currentIndex();
    if (index.isValid())
        emit chatContact(index.data(RosterModel::BareJidRole).toString());
}

