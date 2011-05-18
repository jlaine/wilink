/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#ifndef __WILINK_CONTACTS_H__
#define __WILINK_CONTACTS_H__

#include <QObject>

class Chat;
class QAbstractButton;
class QXmppPresence;

class ContactsWatcher : public QObject
{
    Q_OBJECT

public:
    ContactsWatcher(Chat *chatWindow);

private slots:
    void presenceHandled(QAbstractButton*);
    void presenceReceived(const QXmppPresence &presence);

private:
    Chat *chat;
};

#endif
