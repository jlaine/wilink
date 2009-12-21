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

#include "qxmpp/QXmppRoster.h"

class QContextMenuEvent;
class QXmppClient;
class QXmppVCard;
class QXmppVCardManager;

class RosterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        BareJidRole = Qt::UserRole,
    };

    RosterModel(QXmppRoster *roster);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected slots:
    void presenceChanged(const QString& bareJid, const QString& resource);
    void rosterChanged(const QString &jid);
    void rosterReceived();
    void vCardReceived(const QXmppVCard&);

private:
    QXmppRoster *modelRoster;
    QStringList rosterKeys;
};

class RosterView : public QListView
{
    Q_OBJECT

public:
    RosterView(QXmppClient &client, QWidget *parent = NULL);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void chatContact(const QString &jid);
    void removeContact(const QString &jid);

protected slots:
    void startChat();
    void removeContact();

private:
    QMenu *contextMenu;
};

#endif
