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

#include <QAuthenticator>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkDiskCache>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include "qnetio/wallet.h"
#include "QXmppUtils.h"

#include "application.h"
#include "config.h"
#include "chat.h"
#include "chat_accounts.h"
#include "chat_utils.h"

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    QList<Chat*> chats;
    QNetworkDiskCache *networkCache;
    QSettings *settings;
#ifdef USE_SYSTRAY
    QWidget *trayContext;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
#endif
    UpdatesDialog *updates;
};

ApplicationPrivate::ApplicationPrivate()
    : settings(0),
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
    QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    qDebug() << "Using data directory" << dataPath;
    QDir().mkpath(dataPath);
    d->networkCache = new QNetworkDiskCache(this);
    d->networkCache->setCacheDirectory(QDir(dataPath).filePath("cache"));
    QNetIO::Wallet::setDataPath(QDir(dataPath).filePath("wallet"));

    /* initialise settings */
    migrateFromWdesktop();
    d->settings = new QSettings(this);
    if (isInstalled() && openAtLogin())
        setOpenAtLogin(true);
}

Application::~Application()
{
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
            this, SLOT(messageClicked()));
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

QAbstractNetworkCache *Application::networkCache()
{
    return d->networkCache;
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

#ifdef WILINK_EMBEDDED
#ifdef Q_OS_SYMBIAN
        chat->showMaximized();
#else
        chat->resize(200, 320);
        chat->show();
#endif
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

void Application::messageClicked()
{
#ifdef USE_SYSTRAY
    emit messageClicked(d->trayContext);
#endif
}

void Application::showMessage(QWidget *context, const QString &title, const QString &message)
{
#ifdef USE_SYSTRAY
    if (d->trayIcon)
    {
        d->trayContext = context;
        d->trayIcon->showMessage(title, message);
    }
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
#endif

UpdatesDialog *Application::updatesDialog()
{
    return d->updates;
}

void Application::setUpdatesDialog(UpdatesDialog *updatesDialog)
{
    d->updates = updatesDialog;
}

