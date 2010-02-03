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

#ifndef __WDESKTOP_CHAT_DIALOG_H__
#define __WDESKTOP_CHAT_DIALOG_H__

#include <QWidget>
#include <QTextBrowser>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppArchiveIq.h"

class ChatEdit;
class ChatHistory;
class QLabel;
class QLineEdit;
class QXmppVCard;

class ChatDialog : public QWidget
{
    Q_OBJECT

public:
    ChatDialog(const QString &jid, QWidget *parent = NULL);
    void setAvatar(const QPixmap &avatar);
    void setLocalName(const QString &name);
    void setRemoteName(const QString &name);

public slots:
    void archiveChatReceived(const QXmppArchiveChat &chat);
    void messageReceived(const QXmppMessage &msg);

protected slots:
    void send();

signals:
    void sendMessage(const QString &jid, const QString &message);

private:
    QLabel *avatarLabel;
    QLabel *nameLabel;
    ChatHistory *chatHistory;
    ChatEdit *chatInput;

    QString chatRemoteJid;
};

#endif
