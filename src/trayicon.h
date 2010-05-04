/*
 * wiLink
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

#ifndef __TRAYICON_H__
#define __TRAYICON_H__

#include <QList>
#include <QSystemTrayIcon>

class Chat;
class UpdatesDialog;

class QAuthenticator;
class QLabel;
class QSettings;

/** A TrayIcon is a system tray icon for interacting with a Panel.
 */
class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayIcon();
    ~TrayIcon();

protected slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void resetChats();
    void showAccounts();
    void showChats();

private:
    QList<Chat*> chats;
    QSettings *settings;
    UpdatesDialog *updates;
};

#endif
