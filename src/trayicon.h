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

#ifndef __TRAYICON_H__
#define __TRAYICON_H__

#include <QDialog>
#include <QList>
#include <QSystemTrayIcon>
#include <QUrl>

#include "updates.h"

class Chat;
class Diagnostics;
class Photos;
class Release;

class QAction;
class QAuthenticator;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;

class UpdatesDialog : public QDialog
{
    Q_OBJECT

public:
    UpdatesDialog(QWidget *parent = NULL);

public slots:
    void check();

protected slots:
    void updateAvailable(const Release &release);
    void updateDownloaded(const QUrl &url);
    void updateProgress(qint64 done, qint64 total);

signals:
    void installRelease(const Release &release);

private:
    QProgressBar *progressBar;
    QLabel *statusLabel;
    Updates *updates;
};


/** A TrayIcon is a system tray icon for interacting with a Panel.
 */
class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayIcon();

protected slots:
    void fetchMenu();
    void getCredentials(const QString &realm, QAuthenticator *authenticator);
    void openUrl();
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void showIcon();
    void showDiagnostics();
    void showMenu();
    void showPhotos();

private:
    void fetchIcon();

private:
    Chat *chat;
    Diagnostics *diagnostics;
    Photos *photos;
    UpdatesDialog *updates;

    bool connected;
    int refreshInterval;
    QNetworkAccessManager *network;
    QList< QPair<QUrl, QAction *> > icons;
    QStringList seenMessages;
};

#endif
