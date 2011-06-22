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
#include <QStringList>
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
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(QString organizationName READ organizationName CONSTANT)
    Q_PROPERTY(QString osType READ osType CONSTANT)
    Q_PROPERTY(QString audioInputDeviceName READ audioInputDeviceName WRITE setAudioInputDeviceName NOTIFY audioInputDeviceChanged)
    Q_PROPERTY(QString audioOutputDeviceName READ audioOutputDeviceName WRITE setAudioOutputDeviceName NOTIFY audioOutputDeviceChanged)
    Q_PROPERTY(QStringList chatAccounts READ chatAccounts WRITE setChatAccounts NOTIFY chatAccountsChanged)
    Q_PROPERTY(QString downloadsLocation READ downloadsLocation CONSTANT)
    Q_PROPERTY(QString incomingMessageSound READ incomingMessageSound WRITE setIncomingMessageSound NOTIFY incomingMessageSoundChanged)
    Q_PROPERTY(QString outgoingMessageSound READ outgoingMessageSound WRITE setOutgoingMessageSound NOTIFY outgoingMessageSoundChanged)
    Q_PROPERTY(bool isInstalled READ isInstalled CONSTANT)
    Q_PROPERTY(bool openAtLogin READ openAtLogin WRITE setOpenAtLogin NOTIFY openAtLoginChanged)
    Q_PROPERTY(bool sharesConfigured READ sharesConfigured WRITE setSharesConfigured NOTIFY sharesConfiguredChanged)
    Q_PROPERTY(QStringList sharesDirectories READ sharesDirectories WRITE setSharesDirectories NOTIFY sharesDirectoriesChanged)
    Q_PROPERTY(QString sharesLocation READ sharesLocation WRITE setSharesLocation NOTIFY sharesLocationChanged)
    Q_PROPERTY(bool showOfflineContacts READ showOfflineContacts WRITE setShowOfflineContacts NOTIFY showOfflineContactsChanged)
    Q_PROPERTY(bool sortContactsByStatus READ sortContactsByStatus WRITE setSortContactsByStatus NOTIFY sortContactsByStatusChanged)
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
    QString osType() const;

    // sound
    QAudioDeviceInfo audioInputDevice() const;
    void setAudioInputDevice(const QAudioDeviceInfo &device);

    QAudioDeviceInfo audioOutputDevice() const;
    void setAudioOutputDevice(const QAudioDeviceInfo &device);

    // preferences
    QString audioInputDeviceName() const;
    void setAudioInputDeviceName(const QString &name);

    QString audioOutputDeviceName() const;
    void setAudioOutputDeviceName(const QString &name);

    QStringList chatAccounts() const;
    void setChatAccounts(const QStringList &accounts);

    QString downloadsLocation() const;

    bool openAtLogin() const;
    void setOpenAtLogin(bool run);

    QString incomingMessageSound() const;
    void setIncomingMessageSound(const QString &soundFile);

    QString outgoingMessageSound() const;
    void setOutgoingMessageSound(const QString &soundFile);

    bool sharesConfigured() const;
    void setSharesConfigured(bool configured);

    QStringList sharesDirectories() const;
    void setSharesDirectories(const QStringList &directories);

    QString sharesLocation() const;
    void setSharesLocation(const QString &location);

    bool showOfflineContacts() const;
    void setShowOfflineContacts(bool show);

    bool sortContactsByStatus() const;
    void setSortContactsByStatus(bool sort);

signals:
    void audioInputDeviceChanged(const QAudioDeviceInfo &device);
    void audioOutputDeviceChanged(const QAudioDeviceInfo &device);
    void chatAccountsChanged(const QStringList &accounts);
    void incomingMessageSoundChanged(const QString &sound);
    void outgoingMessageSoundChanged(const QString &sound);
    void messageClicked(QObject *context);
    void openAtLoginChanged(bool run);
    void sharesConfiguredChanged(bool configured);
    void sharesDirectoriesChanged(const QStringList &directories);
    void sharesLocationChanged(const QString &location);
    void showOfflineContactsChanged(bool show);
    void sortContactsByStatusChanged(bool sort);

public slots:
    void resetWindows();
    void showMessage(QObject *context, const QString &title, const QString &message);
    void showWindows();

private slots:
#ifdef USE_SYSTRAY
    void trayActivated(QSystemTrayIcon::ActivationReason reason);
    void trayClicked();
#endif

private:
    static QString executablePath();
    void migrateFromWdesktop();

    ApplicationPrivate * const d;
};

extern Application *wApp;

#endif
