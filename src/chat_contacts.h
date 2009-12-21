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

#ifndef __WDESKTOP_CHAT_CONTACTS_H__
#define __WDESKTOP_CHAT_CONTACTS_H__

#include <QAbstractListModel>
#include <QListView>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class QContextMenuEvent;
class QXmppVCard;
class QXmppVCardManager;

class RosterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    RosterModel(QXmppRoster *roster);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected slots:
    void rosterChanged(const QString &jid);
    void rosterReceived();

private:
    QXmppRoster *modelRoster;
    QStringList rosterKeys;
};

class ContactsList : public QListView
{
    Q_OBJECT

public:
    ContactsList(QXmppRoster *roster, QXmppVCardManager *cardManager, QWidget *parent = NULL);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void chatContact(const QString &jid);
    void removeContact(const QString &jid);

public slots:
    void presenceReceived(const QXmppPresence &presence);

protected slots:
    void startChat();
    void removeContact();
    void rosterChanged(const QString &jid);
    void vCardReceived(const QXmppVCard&);

private:
    QMenu *contextMenu;
    QXmppVCardManager *vcardManager;
    QXmppRoster *xmppRoster;
};

#endif
