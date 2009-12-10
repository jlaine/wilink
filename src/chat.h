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
#include <QListWidget>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class QContextMenuEvent;
class QLabel;
class QLineEdit;
class QSystemTrayIcon;
class QTextEdit;

class ContactsList : public QListWidget
{
    Q_OBJECT

public:
    ContactsList(QWidget *parent = NULL);
    void addEntry(const QXmppRoster::QXmppRosterEntry &entry);
    void setStatus(const QString &jid, const QXmppPresence::Status &entry);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void chatContact(const QString &jid);
    void removeContact(const QString &jid);

protected slots:
    void slotItemDoubleClicked(QListWidgetItem *item);
    void removeContact();

private:
    QMenu *contextMenu;
};

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog(QWidget *parent, const QString &jid);

public slots:
    void handleMessage(const QXmppMessage &msg);

protected slots:
    void send();

signals:
    void sendMessage(const QString &jid, const QString &message);

private:
    QTextEdit *chatHistory;
    QLineEdit *chatInput;
    QString chatJid;
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
    void handleMessage(const QXmppMessage &msg);
    void handlePresence(const QXmppPresence &presence);
    void removeContact(const QString &jid);
    void rosterChanged(const QString &jid);
    void rosterReceived();

private:
    QXmppClient *client;
    ContactsList *contacts;
    QLabel *statusLabel;
    QSystemTrayIcon *systemTrayIcon;
    QHash<QString, ChatDialog*> chatDialogs;
};


#endif
