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

#ifndef __WDESKTOP_CHAT_H__
#define __WDESKTOP_CHAT_H__

#include <QDialog>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class RosterView;
class QLabel;
class QLineEdit;
class QSystemTrayIcon;
class QTextBrowser;
class QXmppVCard;
class QXmppVCardManager;

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog(QWidget *parent, const QString &jid, const QString &name);

public slots:
    void messageReceived(const QXmppMessage &msg);
    void vCardReceived(const QXmppVCard&);

protected slots:
    void anchorClicked(const QUrl &link);
    void send();

signals:
    void sendMessage(const QString &jid, const QString &message);

private:
    void addMessage(const QString &text, bool local);

    QTextBrowser *chatHistory;
    QLineEdit *chatInput;
    QString chatLocalName;
    QString chatRemoteJid;
    QString chatRemoteName;
};

class Chat : public QDialog
{
    Q_OBJECT

public:
    Chat(QSystemTrayIcon *trayIcon);
    bool open(const QString &jid, const QString &password);

protected slots:
    void addContact();
    ChatDialog *chatContact(const QString &jid);
    void connected();
    void disconnected();
    void messageReceived(const QXmppMessage &msg);
    void presenceReceived(const QXmppPresence &presence);
    void removeContact(const QString &jid);
    void resizeContacts();
    void sendMessage(const QString &jid, const QString message);
    void vCardReceived(const QXmppVCard&);

private:
    QXmppClient *client;
    RosterView *contacts;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QHash<QString, ChatDialog*> chatDialogs;
};


#endif
