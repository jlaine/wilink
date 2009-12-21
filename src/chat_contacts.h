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

#include <QListWidget>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class QContextMenuEvent;
class QXmppVCard;
class QXmppVCardManager;

class ContactsList : public QListWidget
{
    Q_OBJECT

public:
    ContactsList(QWidget *parent = NULL);
    void addEntry(const QXmppRoster::QXmppRosterEntry &entry, QXmppVCardManager &VCardManager);
    void setShowOffline(bool show);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void chatContact(const QString &jid);
    void removeContact(const QString &jid);

public slots:
    void presenceReceived(const QXmppPresence &presence);
    void vCardReceived(const QXmppVCard&);

protected slots:
    void startChat();
    void removeContact();

private:
    QMenu *contextMenu;
    bool showOffline;
};

#endif
