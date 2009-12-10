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

#ifndef __BOC_TRAYICON_H__
#define __BOC_TRAYICON_H__

#include <QList>
#include <QSystemTrayIcon>
#include <QUrl>

class Chat;
class Diagnostics;
class Photos;

class QAction;
class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;

/** A TrayIcon is a system tray icon for interacting with a Panel.
 */
class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayIcon();

protected slots:
    void getCredentials(const QString &realm, QAuthenticator *authenticator);
    void openUrl();
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void showIcon();
    void showDiagnostics();
    void showMenu();
    void uploadPhotos();

private:
    void fetchIcon();

private:
    Chat *chat;
    Diagnostics *diagnostics;
    Photos *photos;
    QNetworkAccessManager *network;
    QList< QPair<QUrl, QAction *> > icons;
};

#endif
