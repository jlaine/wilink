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

#ifndef __WDESKTOP_CHAT_FORM_H__
#define __WDESKTOP_CHAT_FORM_H__

#include <QDialog>

#include "qxmpp/QXmppElement.h"

class QFrame;
class QXmppClient;
class QXmppIq;

class ChatRoomOptions : public QDialog
{
    Q_OBJECT

public:
    ChatRoomOptions(QXmppClient *client, const QString &roomJid, QWidget *parent);

protected slots:
    void iqReceived(const QXmppIq &iq);
    void submit();

private:
    QString chatRoomJid;
    QXmppClient *client;
    QXmppElement form;
    QFrame *frame;
};

#endif
