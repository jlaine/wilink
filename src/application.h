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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <QApplication>
#include <QAudioDeviceInfo>
#include <QString>
#include <QUrl>

#ifdef USE_SYSTRAY
#include <QSystemTrayIcon>
#endif

#define HELP_URL "https://www.wifirst.net/wilink/faq"

class UpdatesDialog;
class QAuthenticator;
class QAbstractNetworkCache;
class QSoundPlayer;
class QSystemTrayIcon;
class QThread;

class ApplicationPrivate;

class Application : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(bool showOfflineContacts READ showOfflineContacts WRITE setShowOfflineContacts NOTIFY showOfflineContactsChanged)
    Q_PROPERTY(QString incomingMessageSound READ incomingMessageSound WRITE setIncomingMessageSound)
    Q_PROPERTY(QString outgoingMessageSound READ outgoingMessageSound WRITE setOutgoingMessageSound)
    Q_PROPERTY(QSoundPlayer* soundPlayer READ soundPlayer CONSTANT)

public:
    Application(int &argc, char **argv);
    ~Application();

    static void alert(QWidget *widget);
    QString cacheDirectory() const;
    static void platformInit();
    void createSystemTrayIcon();
    QSoundPlayer *soundPlayer();
    QThread *soundThread();
    UpdatesDialog *updatesDialog();
    void setUpdatesDialog(UpdatesDialog *updatesDialog);
#ifdef USE_SYSTRAY
    QSystemTrayIcon *trayIcon();
#endif

    bool isInstalled();

    // preferences
    bool openAtLogin() const;
    void setOpenAtLogin(bool run);
    QString incomingMessageSound() const;
    void setIncomingMessageSound(const QString &soundFile);
    QString outgoingMessageSound() const;
    void setOutgoingMessageSound(const QString &soundFile);
    bool showOfflineContacts() const;
    void setShowOfflineContacts(bool show);
    QAudioDeviceInfo audioInputDevice() const;
    void setAudioInputDevice(const QAudioDeviceInfo &device);
    QAudioDeviceInfo audioOutputDevice() const;
    void setAudioOutputDevice(const QAudioDeviceInfo &device);

signals:
    void audioInputDeviceChanged(const QAudioDeviceInfo &device);
    void audioOutputDeviceChanged(const QAudioDeviceInfo &device);
    void messageClicked(QObject *context);
    void openAtLoginChanged(bool run);
    void showOfflineContactsChanged(bool show);

public slots:
    void openUrl(const QUrl &url);
    void showMessage(QObject *context, const QString &title, const QString &message);

private slots:
    void messageClicked();
    void resetChats();
    void showAccounts();
    void showChats();
#ifdef USE_SYSTRAY
    void trayActivated(QSystemTrayIcon::ActivationReason reason);
#endif

private:
    static QString executablePath();
    void migrateFromWdesktop();

    ApplicationPrivate * const d;
};

extern Application *wApp;

#endif
