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

#ifndef __WDESKTOP_CHAT_ROOM_H__
#define __WDESKTOP_CHAT_ROOM_H__

#include "chat_dialog.h"

#include <QDialog>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QXmppClient;
class QXmppMessage;

class ChatRoom : public ChatDialog
{
    Q_OBJECT

public:
    ChatRoom(const QString &jid, QWidget *parent = NULL);
    virtual bool isRoom() const;
    virtual void messageReceived(const QXmppMessage &msg);

protected:
    virtual void sendMessage(const QString &text);
};

class ChatRoomOptions : public QDialog
{
    Q_OBJECT

public:
    ChatRoomOptions(QXmppClient *client, const QString &roomJid, QWidget *parent);

protected slots:
    void iqReceived(const QXmppIq &iq);

private:
    QString chatRoomJid;
    QXmppElement form;
};

class ChatRoomPrompt : public QDialog
{
    Q_OBJECT

public:
    ChatRoomPrompt(QXmppClient *client, const QString &roomServer, QWidget *parent);
    QString textValue() const;

protected slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void itemClicked(QListWidgetItem * item);
    void validate();

private:
    QLineEdit *lineEdit;
    QListWidget *listWidget;
    QString chatRoomServer;
};

#endif
