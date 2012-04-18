/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifdef USE_LIBNOTIFY
#include <libnotify/notify.h>
#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif
#endif

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsObject>
#include <QMenu>
#include <QNetworkDiskCache>
#include <QSettings>
#include <QSslSocket>
#include <QSystemTrayIcon>
#include <QThread>
#include <QUrl>

#include "wallet.h"
#include "QSoundPlayer.h"

#include "application.h"
#include "declarative.h"
#include "systeminfo.h"
#include "updater.h"

#ifdef Q_OS_MAC
extern bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret);
#endif

Application *wApp = 0;

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    ApplicationSettings *appSettings;
    QUrl qmlRoot;
    QSoundPlayer *soundPlayer;
    QThread *soundThread;
#ifdef USE_LIBNOTIFY
    bool libnotify_accepts_actions;
#endif
#ifdef USE_SYSTRAY
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    Notification *trayNotification;
#endif
    Updater *updater;
};

ApplicationPrivate::ApplicationPrivate()
    : appSettings(0),
    qmlRoot("qrc:/"),
#ifdef USE_LIBNOTIFY
    libnotify_accepts_actions(0),
#endif
#ifdef USE_SYSTRAY
    trayIcon(0),
    trayMenu(0),
    trayNotification(0),
#endif
    updater(0)
{
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv),
    d(new ApplicationPrivate)
{
    bool check;
    Q_UNUSED(check);

    wApp = this;

    // process command line argument
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-qmlroot") && i < argc - 1) {
            d->qmlRoot = QUrl(QString::fromLocal8Bit(argv[++i]));
        }
    }

    // set application properties
    setApplicationName("wiLink");
    setApplicationVersion(WILINK_VERSION);
    setOrganizationDomain("wifirst.net");
    setOrganizationName("Wifirst");
    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(QIcon(":/wiLink.png"));
#endif

    // initialise settings
    d->appSettings = new ApplicationSettings(this);
    if (isInstalled() && d->appSettings->openAtLogin())
        d->appSettings->setOpenAtLogin(true);

    // initialise cache and wallet
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir().mkpath(dataPath);
    const QString lastRunVersion = d->appSettings->lastRunVersion();
    if (lastRunVersion.isEmpty() || Updater::compareVersions(lastRunVersion, "1.1.900") < 0) {
        QNetworkDiskCache cache;
        cache.setCacheDirectory(QDir(dataPath).filePath("cache"));
        cache.clear();
    }
    d->appSettings->setLastRunVersion(WILINK_VERSION);
    QNetIO::Wallet::setDataPath(QDir(dataPath).filePath("wallet"));

    // FIXME: register URL handler
    //QDesktopServices::setUrlHandler("xmpp", this, "openUrl");

    // initialise sound player
    d->soundThread = new QThread(this);
    d->soundThread->start();
    d->soundPlayer = new QSoundPlayer;
    d->soundPlayer->setInputDeviceName(d->appSettings->audioInputDeviceName());
    check = connect(d->appSettings, SIGNAL(audioInputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setInputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setOutputDeviceName(d->appSettings->audioOutputDeviceName());
    check = connect(d->appSettings, SIGNAL(audioOutputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setOutputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setNetworkAccessManager(new NetworkAccessManager(d->soundPlayer));
    d->soundPlayer->moveToThread(d->soundThread);

#ifdef USE_LIBNOTIFY
    /* initialise libnotify */
    notify_init(applicationName().toUtf8().constData());

    // test if callbacks are supported
    d->libnotify_accepts_actions = false;
    GList *capabilities = notify_get_server_caps();
    if(capabilities != NULL) {
        for(GList *c = capabilities; c != NULL; c = c->next) {
            if(g_strcmp0((char*)c->data, "actions") == 0 ) {
                d->libnotify_accepts_actions = true;
                break;
            }
        }
        g_list_foreach(capabilities, (GFunc)g_free, NULL);
        g_list_free(capabilities);
    }
#endif

    // add SSL root CA for wifirst.net and download.wifirst.net
    QSslSocket::addDefaultCaCertificates(":/UTN_USERFirst_Hardware_Root_CA.pem");

    // create software updater
    d->updater = new Updater;
}

Application::~Application()
{
#ifdef USE_LIBNOTIFY
    // uninitialise libnotify
    notify_uninit();
#endif

#ifdef USE_SYSTRAY
    // destroy tray icon
    if (d->trayIcon)
    {
#ifdef Q_OS_WIN
        // FIXME : on Windows, deleting the icon crashes the program
        d->trayIcon->hide();
#else
        delete d->trayIcon;
        delete d->trayMenu;
#endif
    }
#endif

    // stop sound player
    d->soundThread->quit();
    d->soundThread->wait();
    delete d->soundPlayer;

    delete d;
}

#ifndef Q_OS_MAC
void Application::alert(QWidget *widget)
{
    QApplication::alert(widget);
}

void Application::platformInit()
{
}
#endif

QString Application::cacheDirectory() const
{
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    return QDir(dataPath).filePath("cache");
}

/** Create the system tray icon.
 */
void Application::createSystemTrayIcon()
{
#ifdef USE_SYSTRAY
    d->trayIcon = new QSystemTrayIcon;
#ifdef Q_OS_MAC
    d->trayIcon->setIcon(QIcon(":/wiLink-black.png"));
#else
    d->trayIcon->setIcon(QIcon(":/wiLink.png"));
#endif

    d->trayMenu = new QMenu;
    QAction *action = d->trayMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered()),
            this, SLOT(quit()));
    d->trayIcon->setContextMenu(d->trayMenu);

    connect(d->trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
    connect(d->trayIcon, SIGNAL(messageClicked()),
            this, SLOT(trayClicked()));
    d->trayIcon->show();
#endif
}

QString Application::executablePath() const
{
#ifdef Q_OS_MAC
    const QString macDir("/Contents/MacOS");
    const QString appDir = applicationDirPath();
    if (appDir.endsWith(macDir))
        return appDir.left(appDir.size() - macDir.size());
#endif
#ifdef Q_OS_WIN
    return applicationFilePath().replace("/", "\\");
#endif
    return applicationFilePath();
}

QUrl Application::homeUrl() const
{
    return QUrl::fromLocalFile(QDir::homePath());
}

bool Application::isInstalled() const
{
    QDir dir = QFileInfo(executablePath()).dir();
    return !dir.exists("CMakeFiles");
}

bool Application::isMobile() const
{
#ifdef WILINK_EMBEDDED
    return true;
#else
    return false;
#endif
}

QString Application::osType() const
{
    return SystemInfo::osType();
}

QUrl Application::qmlUrl(const QString &name) const
{
    return d->qmlRoot.resolved(QUrl(name));
}

#if 0
/** Open an XMPP URI using the appropriate account.
 *
 * @param url
 */
void Application::openUrl(const QUrl &url)
{
    foreach (Window *chat, d->chats)
    {
        if (chat->client()->configuration().jidBare() == url.authority()) {
            QUrl simpleUrl = url;
            simpleUrl.setAuthority(QString());
            chat->openUrl(url);
        }
    }
}
#endif

ApplicationSettings* Application::settings() const
{
    return d->appSettings;
}

QUrl Application::resolvedUrl(const QUrl &url, const QUrl &base)
{
    return base.resolved(url);
}

QSoundPlayer *Application::soundPlayer()
{
    return d->soundPlayer;
}

QThread *Application::soundThread()
{
    return d->soundThread;
}

#ifdef USE_LIBNOTIFY
static void notificationClicked(NotifyNotification *notification, char *action, gpointer data)
{
    if(g_strcmp0(action, "show-conversation") == 0)
    {
        Notification *handle = static_cast<Notification*>(data);
        QMetaObject::invokeMethod(handle, "clicked");
        notify_notification_close(notification, NULL);
    }
}

static void notificationClosed(NotifyNotification *notification)
{
    if(notification)
    {
        g_object_unref(G_OBJECT(notification));
    }
}
#endif

#ifndef Q_OS_MAC
Notification *Application::showMessage(const QString &title, const QString &message, const QString &action)
{
    Notification *handle = 0;

#if defined(USE_LIBNOTIFY)
    NotifyNotification *notification = notify_notification_new((const char *)title.toUtf8(),
                                                               (const char *)message.toUtf8(),
#if !NOTIFY_CHECK_VERSION(0, 7, 0)
                                                               NULL,
#endif
                                                               NULL);

    if( !notification ) {
        qWarning("Failed to create notification");
        return 0;
    }

    // Set timeout
    notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);

    // set action handled when notification is closed
    g_signal_connect(notification, "closed", G_CALLBACK(notificationClosed), NULL);

    // Set callback if supported
    if(d->libnotify_accepts_actions) {
        handle = new Notification(this);
        notify_notification_add_action(notification,
                                       "show-conversation",
                                       action.toUtf8(),
                                       (NotifyActionCallback) &notificationClicked,
                                       handle,
                                       FALSE);
    }

    // Schedule notification for showing
    if (!notify_notification_show(notification, NULL))
        qDebug("Failed to send notification");

#elif defined(USE_SYSTRAY)
    if (d->trayIcon)
    {
        handle = new Notification(this);
        if (d->trayNotification)
            delete d->trayNotification;
        d->trayNotification = handle;
        d->trayIcon->showMessage(title, message);
    }
#else
    Q_UNUSED(title);
    Q_UNUSED(message);
    Q_UNUSED(action);
#endif
    return handle;
}
#endif

#ifdef USE_SYSTRAY
void Application::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        emit showWindows();
}

void Application::trayClicked()
{
    if (d->trayNotification) {
        QMetaObject::invokeMethod(d->trayNotification, "clicked");
        d->trayNotification = 0;
    }
}

QSystemTrayIcon *Application::trayIcon()
{
    return d->trayIcon;
}
#endif

Updater *Application::updater() const
{
    return d->updater;
}

class ApplicationSettingsPrivate
{
public:
    QSettings *settings;
};

ApplicationSettings::ApplicationSettings(QObject *parent)
    : QObject(parent),
    d(new ApplicationSettingsPrivate)
{
    d->settings = new QSettings(this);
}

/** Returns the name of the audio input device.
 */
QString ApplicationSettings::audioInputDeviceName() const
{
    return d->settings->value("AudioInputDevice").toString();
}

/** Sets the name of the audio input device.
 *
 * @param name
 */
void ApplicationSettings::setAudioInputDeviceName(const QString &name)
{
    if (name != audioInputDeviceName()) {
        d->settings->setValue("AudioInputDevice", name);
        emit audioInputDeviceNameChanged(name);
    }
}

/** Returns the name of the audio output device.
 */
QString ApplicationSettings::audioOutputDeviceName() const
{
    return d->settings->value("AudioOutputDevice").toString();
}

/** Sets the name of the audio output device.
 *
 * @param name
 */
void ApplicationSettings::setAudioOutputDeviceName(const QString &name)
{
    if (name != audioOutputDeviceName()) {
        d->settings->setValue("AudioOutputDevice", name);
        emit audioOutputDeviceNameChanged(name);
    }
}

/** Returns the list of disabled plugins.
 */
QStringList ApplicationSettings::disabledPlugins() const
{
    return d->settings->value("DisabledPlugins").toStringList();
}

/** Sets the list of disabled plugins.
 *
 * @param plugins
 */
void ApplicationSettings::setDisabledPlugins(const QStringList &plugins)
{
    if (plugins != disabledPlugins()) {
        d->settings->setValue("DisabledPlugins", plugins);
        emit disabledPluginsChanged(plugins);
    }
}

/** Returns the directory where downloaded files are stored.
 */
QString ApplicationSettings::downloadsLocation() const
{
    QStringList dirNames = QStringList() << "Downloads" << "Download";
    foreach (const QString &dirName, dirNames)
    {
        QDir downloads(QDir::home().filePath(dirName));
        if (downloads.exists())
            return downloads.absolutePath();
    }
#ifdef Q_OS_WIN
    return QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
#endif
    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
}

/** Returns the list of enabled plugins.
 */
QStringList ApplicationSettings::enabledPlugins() const
{
    return d->settings->value("EnabledPlugins").toStringList();
}

/** Sets the list of enabled plugins.
 *
 * @param plugins
 */
void ApplicationSettings::setEnabledPlugins(const QStringList &plugins)
{
    if (plugins != enabledPlugins()) {
        d->settings->setValue("EnabledPlugins", plugins);
        emit enabledPluginsChanged(plugins);
    }
}

/** Returns whether a notification should be shown for incoming messages.
 */
bool ApplicationSettings::incomingMessageNotification() const
{
    return d->settings->value("IncomingMessageNotification", true).toBool();
}

/** Sets whether a notification should be shown for incoming messages.
 *
 * @param notification
 */
void ApplicationSettings::setIncomingMessageNotification(bool notification)
{
    if (notification != incomingMessageNotification()) {
        d->settings->setValue("IncomingMessageNotification", notification);
        emit incomingMessageNotificationChanged(notification);
    }
}

/** Returns the sound to play for incoming messages.
 */
QString ApplicationSettings::incomingMessageSound() const
{
    return d->settings->value("IncomingMessageSound").toString();
}

/** Sets the sound to play for incoming messages.
 *
 * @param soundFile
 */
void ApplicationSettings::setIncomingMessageSound(const QString &soundFile)
{
    if (soundFile != incomingMessageSound()) {
        d->settings->setValue("IncomingMessageSound", soundFile);
        emit incomingMessageSoundChanged(soundFile);
    }
}

/** Returns the last version of the application which was run.
 */
QString ApplicationSettings::lastRunVersion() const
{
    return d->settings->value("LastRunVersion").toString();
}

/** Sets the last version of the application which was run.
 *
 * @param version
 */
void ApplicationSettings::setLastRunVersion(const QString &version)
{
    if (version != lastRunVersion()) {
        d->settings->setValue("LastRunVersion", version);
        emit lastRunVersionChanged(version);
    }
}

/** Returns whether the application should be run at login.
 */
bool ApplicationSettings::openAtLogin() const
{
    return d->settings->value("OpenAtLogin", true).toBool();
}

/** Sets whether the application should be run at login.
 *
 * @param run
 */
void ApplicationSettings::setOpenAtLogin(bool run)
{
    const QString appName = wApp->applicationName();
    const QString appPath = wApp->executablePath();
#if defined(Q_OS_MAC)
    QString script = run ?
        QString("tell application \"System Events\"\n"
            "\tmake login item at end with properties {path:\"%1\"}\n"
            "end tell\n").arg(appPath) :
        QString("tell application \"System Events\"\n"
            "\tdelete login item \"%1\"\n"
            "end tell\n").arg(appName);
    qt_mac_execute_apple_script(script, 0);
#elif defined(Q_OS_WIN)
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (run)
        registry.setValue(appName, appPath);
    else
        registry.remove(appName);
#else
    QDir autostartDir(QDir::home().filePath(".config/autostart"));
    if (autostartDir.exists())
    {
        QFile desktop(autostartDir.filePath(appName + ".desktop"));
        const QString fileName = appName + ".desktop";
        if (run)
        {
            if (desktop.open(QIODevice::WriteOnly))
            {
                QTextStream stream(&desktop);
                stream << "[Desktop Entry]\n";
                stream << QString("Name=%1\n").arg(appName);
                stream << QString("Exec=%1\n").arg(appPath);
                stream << "Type=Application\n";
            }
        }
        else
        {
            desktop.remove();
        }
    }
#endif

    // store preference
    if (run != openAtLogin())
    {
        d->settings->setValue("OpenAtLogin", run);
        emit openAtLoginChanged(run);
    }
}


/** Returns the sound to play for outgoing messages.
 */
QString ApplicationSettings::outgoingMessageSound() const
{
    return d->settings->value("OutgoingMessageSound").toString();
}

/** Sets the sound to play for outgoing messages.
 *
 * @param soundFile
 */
void ApplicationSettings::setOutgoingMessageSound(const QString &soundFile)
{
    if (soundFile != outgoingMessageSound()) {
        d->settings->setValue("OutgoingMessageSound", soundFile);
        emit outgoingMessageSoundChanged(soundFile);
    }
}

/** Returns the list of recently used media URLs.
 */
QStringList ApplicationSettings::playerUrls() const
{
    return d->settings->value("PlayerUrls").toStringList();
}

/** Sets the list of recently used media URLs.
 *
 * @param urls
 */
void ApplicationSettings::setPlayerUrls(const QStringList &urls)
{
    if (urls != playerUrls()) {
        d->settings->setValue("PlayerUrls", urls);
        emit playerUrlsChanged(urls);
    }
}

/** Returns true if shares have been configured.
 */
bool ApplicationSettings::sharesConfigured() const
{
    return d->settings->value("SharesConfigured", false).toBool();
}

/** Sets whether shares have been configured.
 */
void ApplicationSettings::setSharesConfigured(bool configured)
{
    if (configured != sharesConfigured()) {
        d->settings->setValue("SharesConfigured", configured);
        emit sharesConfiguredChanged(configured);
    }
}

/** Returns the list of shared directories.
 */
QVariantList ApplicationSettings::sharesDirectories() const
{
    QVariantList urls;
    foreach (const QString &path, d->settings->value("SharesDirectories").toStringList())
        urls << QUrl::fromLocalFile(path);
    return urls;
}

/** Sets the list of shared directories.
 *
 * @param directories
 */
void ApplicationSettings::setSharesDirectories(const QVariantList &directories)
{
    if (directories != sharesDirectories()) {
        QStringList dirs;
        foreach (const QVariant &var, directories)
            dirs << var.toUrl().toLocalFile();
        d->settings->setValue("SharesDirectories", dirs);
        emit sharesDirectoriesChanged(sharesDirectories());
    }
}

/** Returns the base share directory.
 */
QString ApplicationSettings::sharesLocation() const
{
    QString sharesDirectory = d->settings->value("SharesLocation",  QDir::home().filePath("Public")).toString();
    if (sharesDirectory.endsWith("/"))
        sharesDirectory.chop(1);
    return sharesDirectory;
}

/** Sets the base share directory.
 *
 * @param location
 */
void ApplicationSettings::setSharesLocation(const QString &location)
{
    if (location != sharesLocation()) {
        d->settings->setValue("SharesLocation", location);
        emit sharesLocationChanged(sharesLocation());
    }
}

/** Returns the base share directory URL.
 */
QUrl ApplicationSettings::sharesUrl() const
{
    return QUrl::fromLocalFile(sharesLocation());
}

/** Returns true if offline contacts should be displayed.
 */
bool ApplicationSettings::showOfflineContacts() const
{
    return d->settings->value("ShowOfflineContacts", true).toBool();
}

/** Sets whether offline contacts should be displayed.
 *
 * @param show
 */
void ApplicationSettings::setShowOfflineContacts(bool show)
{
    if (show != showOfflineContacts()) {
        d->settings->setValue("ShowOfflineContacts", show);
        emit showOfflineContactsChanged(show);
    }
}

/** Returns true if offline contacts should be displayed.
 */
bool ApplicationSettings::sortContactsByStatus() const
{
    return d->settings->value("SortContactsByStatus", true).toBool();
}

/** Sets whether contacts should be sorted by status.
 *
 * @param sort
 */
void ApplicationSettings::setSortContactsByStatus(bool sort)
{
    if (sort != sortContactsByStatus())
    {
        d->settings->setValue("SortContactsByStatus", sort);
        emit sortContactsByStatusChanged(sort);
    }
}

