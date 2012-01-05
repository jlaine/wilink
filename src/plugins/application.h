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
#include <QString>
#include <QStringList>
#include <QUrl>

#ifdef USE_SYSTRAY
#include <QSystemTrayIcon>
#endif

#define HELP_URL "https://www.wifirst.net/wilink/faq"

class UpdateDialog;
class Updater;
class QAuthenticator;
class QAbstractNetworkCache;
class QSoundPlayer;
class QSystemTrayIcon;
class QThread;

class ApplicationPrivate;
class ApplicationSettings;
class ApplicationSettingsPrivate;

class Notification : public QObject
{
    Q_OBJECT

public:
    Notification(QObject *parent = 0) : QObject(parent)
    {}

signals:
    void clicked();
};

class Application : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(QString organizationName READ organizationName CONSTANT)
    Q_PROPERTY(QString osType READ osType CONSTANT)
    Q_PROPERTY(bool isInstalled READ isInstalled CONSTANT)
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(ApplicationSettings* settings READ settings CONSTANT)
    Q_PROPERTY(QSoundPlayer* soundPlayer READ soundPlayer CONSTANT)
    Q_PROPERTY(Updater* updater READ updater CONSTANT)

public:
    Application(int &argc, char **argv);
    ~Application();

    static void alert(QWidget *widget);
    QString cacheDirectory() const;
    static void platformInit();
    void createSystemTrayIcon();
    QSoundPlayer *soundPlayer();
    QThread *soundThread();
    Updater *updater() const;
#ifdef USE_SYSTRAY
    QSystemTrayIcon *trayIcon();
#endif

    QString executablePath() const;
    QUrl qmlUrl(const QString &name) const;

    bool isInstalled() const;
    bool isMobile() const;
    QString osType() const;

    // preferences
    ApplicationSettings *settings() const;

public slots:
    void resetWindows();
    QUrl resolvedUrl(const QUrl &url, const QUrl &base);
    Notification *showMessage(const QString &title, const QString &message, const QString &action);
    void showWindows();

private slots:
#ifdef USE_SYSTRAY
    void trayActivated(QSystemTrayIcon::ActivationReason reason);
    void trayClicked();
#endif

private:

    ApplicationPrivate * const d;
};

/** The ApplicationSettings class represents the Application's settings.
 */
class ApplicationSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString audioInputDeviceName READ audioInputDeviceName WRITE setAudioInputDeviceName NOTIFY audioInputDeviceNameChanged)
    Q_PROPERTY(QString audioOutputDeviceName READ audioOutputDeviceName WRITE setAudioOutputDeviceName NOTIFY audioOutputDeviceNameChanged)
    Q_PROPERTY(QStringList chatAccounts READ chatAccounts WRITE setChatAccounts NOTIFY chatAccountsChanged)
    Q_PROPERTY(QStringList disabledPlugins READ disabledPlugins WRITE setDisabledPlugins NOTIFY disabledPluginsChanged)
    Q_PROPERTY(QString downloadsLocation READ downloadsLocation CONSTANT)
    Q_PROPERTY(QStringList enabledPlugins READ enabledPlugins WRITE setEnabledPlugins NOTIFY enabledPluginsChanged)
    Q_PROPERTY(bool incomingMessageNotification READ incomingMessageNotification WRITE setIncomingMessageNotification NOTIFY incomingMessageNotificationChanged)
    Q_PROPERTY(QString incomingMessageSound READ incomingMessageSound WRITE setIncomingMessageSound NOTIFY incomingMessageSoundChanged)
    Q_PROPERTY(QString lastRunVersion READ lastRunVersion WRITE setLastRunVersion NOTIFY lastRunVersionChanged)
    Q_PROPERTY(bool openAtLogin READ openAtLogin WRITE setOpenAtLogin NOTIFY openAtLoginChanged)
    Q_PROPERTY(QString outgoingMessageSound READ outgoingMessageSound WRITE setOutgoingMessageSound NOTIFY outgoingMessageSoundChanged)
    Q_PROPERTY(QStringList playerUrls READ playerUrls WRITE setPlayerUrls NOTIFY playerUrlsChanged)
    Q_PROPERTY(bool sharesConfigured READ sharesConfigured WRITE setSharesConfigured NOTIFY sharesConfiguredChanged)
    Q_PROPERTY(QStringList sharesDirectories READ sharesDirectories WRITE setSharesDirectories NOTIFY sharesDirectoriesChanged)
    Q_PROPERTY(QString sharesLocation READ sharesLocation WRITE setSharesLocation NOTIFY sharesLocationChanged)
    Q_PROPERTY(bool showOfflineContacts READ showOfflineContacts WRITE setShowOfflineContacts NOTIFY showOfflineContactsChanged)
    Q_PROPERTY(bool sortContactsByStatus READ sortContactsByStatus WRITE setSortContactsByStatus NOTIFY sortContactsByStatusChanged)

public:
    ApplicationSettings(QObject *parent = 0);

    QString audioInputDeviceName() const;
    void setAudioInputDeviceName(const QString &name);

    QString audioOutputDeviceName() const;
    void setAudioOutputDeviceName(const QString &name);

    QStringList chatAccounts() const;
    void setChatAccounts(const QStringList &accounts);

    QStringList disabledPlugins() const;
    void setDisabledPlugins(const QStringList &plugins);

    QString downloadsLocation() const;

    QStringList enabledPlugins() const;
    void setEnabledPlugins(const QStringList &plugins);

    bool incomingMessageNotification() const;
    void setIncomingMessageNotification(bool notification);

    QString incomingMessageSound() const;
    void setIncomingMessageSound(const QString &soundFile);

    QString lastRunVersion() const;
    void setLastRunVersion(const QString &version);

    bool openAtLogin() const;
    void setOpenAtLogin(bool run);

    QString outgoingMessageSound() const;
    void setOutgoingMessageSound(const QString &soundFile);

    QStringList playerUrls() const;
    void setPlayerUrls(const QStringList &urls);

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

    QByteArray windowGeometry(const QString &key) const;
    void setWindowGeometry(const QString &key, const QByteArray &geometry);

signals:
    /// \cond
    void audioInputDeviceNameChanged(const QString &name);
    void audioOutputDeviceNameChanged(const QString &name);
    void chatAccountsChanged(const QStringList &accounts);
    void disabledPluginsChanged(const QStringList &plugins);
    void enabledPluginsChanged(const QStringList &plugins);
    void incomingMessageNotificationChanged(bool notification);
    void incomingMessageSoundChanged(const QString &sound);
    void lastRunVersionChanged(const QString &version);
    void openAtLoginChanged(bool run);
    void outgoingMessageSoundChanged(const QString &sound);
    void playerUrlsChanged(const QStringList &urls);
    void sharesConfiguredChanged(bool configured);
    void sharesDirectoriesChanged(const QStringList &directories);
    void sharesLocationChanged(const QString &location);
    void showOfflineContactsChanged(bool show);
    void sortContactsByStatusChanged(bool sort);
    /// \endcond

private:
    ApplicationSettingsPrivate *d;
};

extern Application *wApp;

#endif
