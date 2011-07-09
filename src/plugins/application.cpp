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

#ifdef USE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsObject>
#include <QMenu>
#include <QNetworkDiskCache>
#include <QProcess>
#include <QSettings>
#include <QSslSocket>
#include <QSystemTrayIcon>
#include <QThread>
#include <QUrl>

#include "wallet.h"
#include "QSoundPlayer.h"

#include "application.h"
#include "systeminfo.h"
#include "updater.h"
#include "window.h"

Application *wApp = 0;

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    QList<QWidget*> chats;
    QSettings *oldSettings;
    ApplicationSettings *appSettings;
    QSoundPlayer *soundPlayer;
    QThread *soundThread;
    QAudioDeviceInfo audioInputDevice;
    QAudioDeviceInfo audioOutputDevice;
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
    : oldSettings(0),
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
    wApp = this;
    qRegisterMetaType<QAudioDeviceInfo>();

    /* set application properties */
    setApplicationName("wiLink");
    setApplicationVersion(WILINK_VERSION);
    setOrganizationDomain("wifirst.net");
    setOrganizationName("Wifirst");
    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(QIcon(":/wiLink.png"));
#endif

    /* initialise settings */
    d->oldSettings = new QSettings(this);
    d->appSettings = new ApplicationSettings(this);

    if (isInstalled() && openAtLogin())
        setOpenAtLogin(true);

    /* initialise cache and wallet */
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

    /* initialise sound player */
    d->soundThread = new QThread(this);
    d->soundThread->start();
    d->soundPlayer = new QSoundPlayer;
    d->soundPlayer->setAudioOutputDevice(d->audioOutputDevice);
    connect(wApp, SIGNAL(audioOutputDeviceChanged(QAudioDeviceInfo)),
            d->soundPlayer, SLOT(setAudioOutputDevice(QAudioDeviceInfo)));
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
#ifndef WILINK_EMBEDDED
    // save window geometry
    foreach (QWidget *chat, d->chats) {
        const QString key = chat->objectName();
        if (!key.isEmpty()) {
            d->appSettings->setWindowGeometry(key, chat->saveGeometry());
        }
    }
#endif

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

    // destroy chat windows
    foreach (QWidget *chat, d->chats)
        delete chat;

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
    d->trayIcon->setIcon(QIcon(":/wiLink.png"));

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

QString Application::executablePath()
{
#ifdef Q_OS_MAC
    const QString macDir("/Contents/MacOS");
    const QString appDir = qApp->applicationDirPath();
    if (appDir.endsWith(macDir))
        return appDir.left(appDir.size() - macDir.size());
#endif
#ifdef Q_OS_WIN
    return qApp->applicationFilePath().replace("/", "\\");
#endif
    return qApp->applicationFilePath();
}

bool Application::isInstalled()
{
    QDir dir = QFileInfo(executablePath()).dir();
    return !dir.exists("CMakeFiles");
}

QString Application::osType() const
{
    return SystemInfo::osType();
}

bool Application::openAtLogin() const
{
    return d->oldSettings->value("OpenAtLogin", true).toBool();
}

void Application::setOpenAtLogin(bool run)
{
    const QString appName = qApp->applicationName();
    const QString appPath = executablePath();
#if defined(Q_OS_MAC)
    QString script = run ?
        QString("tell application \"System Events\"\n"
            "\tmake login item at end with properties {path:\"%1\"}\n"
            "end tell\n").arg(appPath) :
        QString("tell application \"System Events\"\n"
            "\tdelete login item \"%1\"\n"
            "end tell\n").arg(appName);
    QProcess process;
    process.start("osascript");
    process.write(script.toAscii());
    process.closeWriteChannel();
    process.waitForFinished();
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
        d->oldSettings->setValue("OpenAtLogin", run);
        emit openAtLoginChanged(run);
    }
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

void Application::resetWindows()
{
    /* close any existing windows */
    foreach (QWidget *chat, d->chats)
        chat->deleteLater();
    d->chats.clear();

    /* check we have a valid account */
    if (d->appSettings->chatAccounts().isEmpty()) {

        Window *window = new Window(QUrl("qrc:/setup.qml"), QString());

        const QSize size = QApplication::desktop()->availableGeometry(window).size();
        window->move((size.width() - window->width()) / 2, (size.height() - window->height()) / 2);
        window->show();
        window->raise();
        d->chats << window;
        return;
    }

    /* connect to chat accounts */
    int xpos = 30;
    int ypos = 20;
    const QStringList chatJids = d->appSettings->chatAccounts();
    foreach (const QString &jid, chatJids) {
        Window *window = new Window(QUrl("qrc:/main.qml"), jid);

#ifdef WILINK_EMBEDDED
        Q_UNUSED(xpos);
        Q_UNUSED(ypos);
        window->showFullScreen();
#else
        // restore window geometry
        const QByteArray geometry = d->appSettings->windowGeometry(jid);
        if (!geometry.isEmpty()) {
            window->restoreGeometry(geometry);
        } else {
            QSize size = QApplication::desktop()->availableGeometry(window).size();
            size.setHeight(size.height() - 100);
            size.setWidth((size.height() * 4.0) / 3.0);
            window->resize(size);
            window->move(xpos, ypos);
        }
        window->show();
#endif
        window->raise();
        window->activateWindow();

        d->chats << window;
        xpos += 100;
    }
}

void Application::showWindows()
{
    foreach (QWidget *chat, d->chats) {
        chat->setWindowState(chat->windowState() & ~Qt::WindowMinimized);
        chat->show();
        chat->raise();
        chat->activateWindow();
    }
}

QSoundPlayer *Application::soundPlayer()
{
    return d->soundPlayer;
}

QThread *Application::soundThread()
{
    return d->soundThread;
}

QAudioDeviceInfo Application::audioInputDevice() const
{
    const QString &name = d->appSettings->audioInputDeviceName();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == name) {
            return info;
        }
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

QAudioDeviceInfo Application::audioOutputDevice() const
{
    const QString name = d->appSettings->audioOutputDeviceName();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == name) {
            return info;
        }
    }
    return QAudioDeviceInfo::defaultOutputDevice();
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

Notification *Application::showMessage(const QString &title, const QString &message, const QString &action)
{
    Notification *handle = 0;

#if defined(USE_LIBNOTIFY)
    NotifyNotification *notification = notify_notification_new((const char *)title.toUtf8(),
                                                               (const char *)message.toUtf8(),
                                                               NULL,
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

#ifdef USE_SYSTRAY
void Application::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        showWindows();
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

    // clean acounts
    QStringList cleanAccounts = chatAccounts();
    foreach (const QString &jid, cleanAccounts) {
        if (!QRegExp("^[^@/ ]+@[^@/ ]+$").exactMatch(jid)) {
            qWarning("Removing bad account %s", qPrintable(jid));
            cleanAccounts.removeAll(jid);
        }
    }
    setChatAccounts(cleanAccounts);
}

QString ApplicationSettings::audioInputDeviceName() const
{
    return d->settings->value("AudioInputDevice").toString();
}

void ApplicationSettings::setAudioInputDeviceName(const QString &name)
{
    if (name != audioInputDeviceName()) {
        d->settings->setValue("AudioInputDevice", name);
        emit audioInputDeviceNameChanged(name);
    }
}

QString ApplicationSettings::audioOutputDeviceName() const
{
    return d->settings->value("AudioOutputDevice").toString();
}

void ApplicationSettings::setAudioOutputDeviceName(const QString &name)
{
    if (name != audioOutputDeviceName()) {
        d->settings->setValue("AudioOutputDevice", name);
        emit audioOutputDeviceNameChanged(name);
    }
}

/** Returns the list of chat account JIDs.
 */
QStringList ApplicationSettings::chatAccounts() const
{
    return d->settings->value("ChatAccounts").toStringList();
}

/** Sets the list of chat account JIDs.
 *
 * @param accounts
 */
void ApplicationSettings::setChatAccounts(const QStringList &accounts)
{
    if (accounts != chatAccounts()) {
        d->settings->setValue("ChatAccounts", accounts);
        emit chatAccountsChanged(accounts);
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

QString ApplicationSettings::lastRunVersion() const
{
    return d->settings->value("LastRunVersion").toString();
}

void ApplicationSettings::setLastRunVersion(const QString &version)
{
    if (version != lastRunVersion()) {
        d->settings->setValue("LastRunVersion", version);
        emit lastRunVersionChanged(version);
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
QStringList ApplicationSettings::sharesDirectories() const
{
    return d->settings->value("SharesDirectories").toStringList();
}

/** Sets the list of shared directories.
 *
 * @param directories
 */
void ApplicationSettings::setSharesDirectories(const QStringList &directories)
{
    if (directories != sharesDirectories()) {
        d->settings->setValue("SharesDirectories", directories);
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

QByteArray ApplicationSettings::windowGeometry(const QString &key) const
{
    return d->settings->value("WindowGeometry/" + key).toByteArray();
}

void ApplicationSettings::setWindowGeometry(const QString &key, const QByteArray &geometry)
{
    d->settings->setValue("WindowGeometry/" + key, geometry);
}

