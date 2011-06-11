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

#include <QAuthenticator>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>
#include <QSettings>
#include <QSslSocket>
#include <QSystemTrayIcon>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include "qnetio/wallet.h"
#include "QSoundPlayer.h"
#include "QXmppUtils.h"

#include "accounts.h"
#include "application.h"
#include "utils.h"
#include "window.h"

Application *wApp = 0;

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    QList<Chat*> chats;
    QSettings *settings;
    QSoundPlayer *soundPlayer;
    QThread *soundThread;
    QAudioDeviceInfo audioInputDevice;
    QAudioDeviceInfo audioOutputDevice;
#ifdef USE_LIBNOTIFY
    bool libnotify_accepts_actions;
#endif
#ifdef USE_SYSTRAY
    QObject *trayContext;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
#endif
    UpdatesDialog *updates;
};

ApplicationPrivate::ApplicationPrivate()
    : settings(0),
#ifdef USE_LIBNOTIFY
    libnotify_accepts_actions(0),
#endif
#ifdef USE_SYSTRAY
    trayContext(0),
    trayIcon(0),
    trayMenu(0),
#endif
    updates(0)
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

    /* initialise cache and wallet */
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir().mkpath(dataPath);
    QNetIO::Wallet::setDataPath(QDir(dataPath).filePath("wallet"));

    /* initialise settings */
    migrateFromWdesktop();
    d->settings = new QSettings(this);
    if (isInstalled() && openAtLogin())
        setOpenAtLogin(true);
    d->audioInputDevice = QAudioDeviceInfo::defaultInputDevice();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == d->settings->value("AudioInputDevice").toString()) {
            d->audioInputDevice = info;
            break;
        }
    }
    d->audioOutputDevice = QAudioDeviceInfo::defaultOutputDevice();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == d->settings->value("AudioOutputDevice").toString()) {
            d->audioOutputDevice = info;
            break;
        }
    }

    /* initialise style */
    QFile css(":/wiLink.css");
    if (css.open(QIODevice::ReadOnly))
       setStyleSheet(QString::fromUtf8(css.readAll()));

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

    /* add SSL root CA for wifirst.net and download.wifirst.net */
    QSslSocket::addDefaultCaCertificates(":/UTN_USERFirst_Hardware_Root_CA.pem");
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

    // destroy chat windows
    foreach (Chat *chat, d->chats)
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
    QAction *action = d->trayMenu->addAction(QIcon(":/options.png"), tr("Chat accounts"));
    connect(action, SIGNAL(triggered()),
            this, SLOT(showAccounts()));
    action = d->trayMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
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

void Application::migrateFromWdesktop()
{
    /* Disable auto-launch of wDesktop */
#ifdef Q_OS_MAC
    QProcess process;
    process.start("osascript");
    process.write("tell application \"System Events\"\n\tdelete login item \"wDesktop\"\nend tell\n");
    process.closeWriteChannel();
    process.waitForFinished();
#endif
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    settings.remove("wDesktop");
#endif

    /* Migrate old settings */
#ifdef Q_OS_LINUX
    QDir(QDir::home().filePath(".config/Wifirst")).rename("wDesktop.conf",
        QString("%1.conf").arg(qApp->applicationName()));
#endif
#ifdef Q_OS_MAC
    QDir(QDir::home().filePath("Library/Preferences")).rename("com.wifirst.wDesktop.plist",
        QString("net.wifirst.%1.plist").arg(qApp->applicationName()));
#endif
#ifdef Q_OS_WIN
    QSettings oldSettings("HKEY_CURRENT_USER\\Software\\Wifirst", QSettings::NativeFormat);
    if (oldSettings.childGroups().contains("wDesktop"))
    {
        QSettings newSettings;
        oldSettings.beginGroup("wDesktop");
        foreach (const QString &key, oldSettings.childKeys())
            newSettings.setValue(key, oldSettings.value(key));
        oldSettings.endGroup();
        oldSettings.remove("wDesktop");
    }
#endif

    /* Migrate old passwords */
    QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#ifdef Q_OS_LINUX
    QFile oldWallet(QDir::home().filePath("wDesktop"));
    if (oldWallet.exists() && oldWallet.copy(dataDir + "/wallet.dummy"))
        oldWallet.remove();
#endif
#ifdef Q_OS_WIN
    QFile oldWallet(QDir::home().filePath("wDesktop.encrypted"));
    if (oldWallet.exists() && oldWallet.copy(dataDir + "/wallet.windows"))
        oldWallet.remove();
#endif

    /* Uninstall wDesktop */
#ifdef Q_OS_MAC
    QDir appsDir("/Applications");
    if (appsDir.exists("wDesktop.app"))
        QProcess::execute("rm", QStringList() << "-rf" << appsDir.filePath("wDesktop.app"));
#endif
#ifdef Q_OS_WIN
    QString uninstaller("C:\\Program Files\\wDesktop\\Uninstall.exe");
    if (QFileInfo(uninstaller).isExecutable())
        QProcess::execute(uninstaller, QStringList() << "/S");
#endif
}

bool Application::openAtLogin() const
{
    return d->settings->value("OpenAtLogin", true).toBool();
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
        d->settings->setValue("OpenAtLogin", run);
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
    foreach (Chat *chat, d->chats)
    {
        if (chat->client()->configuration().jidBare() == url.authority()) {
            QUrl simpleUrl = url;
            simpleUrl.setAuthority(QString());
            chat->openUrl(url);
        }
    }
}
#endif

QString Application::incomingMessageSound() const
{
    return d->settings->value("IncomingMessageSound").toString();
}

void Application::setIncomingMessageSound(const QString &soundFile)
{
    d->settings->setValue("IncomingMessageSound", soundFile);
}

QString Application::outgoingMessageSound() const
{
    return d->settings->value("OutgoingMessageSound").toString();
}

void Application::setOutgoingMessageSound(const QString &soundFile)
{
    d->settings->setValue("OutgoingMessageSound", soundFile);
}

void Application::showAccounts()
{
    ChatAccounts dlg;
    dlg.exec();

    // reset chats later as we may delete the calling window
    if (dlg.changed())
        QTimer::singleShot(0, this, SLOT(resetChats()));
}

void Application::resetChats()
{
    /* close any existing chats */
    foreach (Chat *chat, d->chats)
        delete chat;
    d->chats.clear();

    /* clean any bad accounts */
    ChatAccounts dlg;
    dlg.check();

    /* connect to chat accounts */
    int xpos = 30;
    int ypos = 20;
    const QStringList chatJids = dlg.accounts();
    foreach (const QString &jid, chatJids)
    {
        Chat *chat = new Chat;
        if (chatJids.size() == 1)
            chat->setWindowTitle(qApp->applicationName());
        else
            chat->setWindowTitle(QString("%1 - %2").arg(jid, qApp->applicationName()));

#ifdef Q_OS_SYMBIAN
        Q_UNUSED(ypos);
        chat->showMaximized();
#else
        chat->move(xpos, ypos);
        chat->show();
#endif

        chat->open(jid);
        d->chats << chat;
        xpos += 300;
    }

    /* show chats */
    showChats();
}

void Application::showChats()
{
    foreach (Chat *chat, d->chats)
    {
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
    return d->audioInputDevice;
}

void Application::setAudioInputDevice(const QAudioDeviceInfo &device)
{
    d->settings->setValue("AudioInputDevice", device.deviceName());
    d->audioInputDevice = device;
    emit audioInputDeviceChanged(device);
}

QAudioDeviceInfo Application::audioOutputDevice() const
{
    return d->audioOutputDevice;
}

void Application::setAudioOutputDevice(const QAudioDeviceInfo &device)
{
    d->settings->setValue("AudioOutputDevice", device.deviceName());
    d->audioOutputDevice = device;
    emit audioOutputDeviceChanged(device);
}

QString Application::downloadsLocation() const
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

#ifdef USE_LIBNOTIFY
static void notificationClicked(NotifyNotification *notification, char *action, gpointer data)
{
    if(g_strcmp0(action, "show-conversation") == 0)
    {
        QMetaObject::invokeMethod(wApp, "messageClicked", Q_ARG(QObject*, (QObject*)data));
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

void Application::showMessage(QObject *context, const QString &title, const QString &message)
{
#if defined(USE_LIBNOTIFY)
    NotifyNotification *notification = notify_notification_new((const char *)title.toUtf8(),
                                                               (const char *)message.toUtf8(),
                                                               NULL,
                                                               NULL);

    if( !notification ) {
        qWarning("Failed to create notification");
        return;
    }

    // Set timeout
    notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);

    // set action handled when notification is closed
    g_signal_connect(notification, "closed", G_CALLBACK(notificationClosed), NULL);

    // Set callback if supported
    if(d->libnotify_accepts_actions) {
        notify_notification_add_action(notification,
                                       "show-conversation",
                                       tr("Show this conversation").toUtf8(),
                                       (NotifyActionCallback) &notificationClicked,
                                       context,
                                       FALSE);
    }

    // Schedule notification for showing
    if (!notify_notification_show(notification, NULL))
        qDebug("Failed to send notification");

#elif defined(USE_SYSTRAY)
    if (d->trayIcon)
    {
        d->trayContext = context;
        d->trayIcon->showMessage(title, message);
    }
#else
    Q_UNUSED(context);
    Q_UNUSED(title);
    Q_UNUSED(message);
#endif
}

/** Returns true if offline contacts should be displayed.
 */
bool Application::showOfflineContacts() const
{
    return d->settings->value("ShowOfflineContacts", true).toBool();
}

/** Sets whether offline contacts should be displayed.
 *
 * @param show
 */
void Application::setShowOfflineContacts(bool show)
{
    if (show != showOfflineContacts())
    {
        d->settings->setValue("ShowOfflineContacts", show);
        emit showOfflineContactsChanged(show);
    }
}

#ifdef USE_SYSTRAY
void Application::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        showChats();
}

void Application::trayClicked()
{
    emit messageClicked(d->trayContext);
}

QSystemTrayIcon *Application::trayIcon()
{
    return d->trayIcon;
}
#endif

UpdatesDialog *Application::updatesDialog()
{
    return d->updates;
}

void Application::setUpdatesDialog(UpdatesDialog *updatesDialog)
{
    d->updates = updatesDialog;
}
