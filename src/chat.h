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

#ifndef __WDESKTOP_CHAT_H__
#define __WDESKTOP_CHAT_H__

#include <QWidget>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class ChatDialog;
class RosterView;
class QLabel;
class QSystemTrayIcon;
class QXmppVCard;
class QXmppVCardManager;

class Chat : public QWidget
{
    Q_OBJECT

public:
    Chat(QSystemTrayIcon *trayIcon);
    bool open(const QString &jid, const QString &password);

protected slots:
    void addContact();
    void chatContact(const QString &jid);
    void connected();
    void disconnected();
    void messageReceived(const QXmppMessage &msg);
    void presenceChanged(const QString& bareJid, const QString& resource);
    void presenceReceived(const QXmppPresence &presence);
    void removeContact(const QString &jid);
    void resizeContacts();
    void sendMessage(const QString &jid, const QString message);

protected:
    ChatDialog *showConversation(const QString &jid);

private:
    QXmppClient *client;
    RosterView *contacts;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QHash<QString, ChatDialog*> chatDialogs;
};


#endif
