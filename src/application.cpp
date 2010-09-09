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
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>

#include "qnetio/wallet.h"
#include "QXmppUtils.h"

#include "application.h"
#include "config.h"
#include "chat.h"
#include "chat_accounts.h"
#include "utils.h"

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv),
    settings(0),
    updates(0),
    trayContext(0),
    trayIcon(0),
    trayMenu(0)
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

    /* initialise wallet */
    QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    qDebug() << "Using data directory" << dataDir;
    QDir().mkpath(dataDir);
    QNetIO::Wallet::setDataPath(dataDir + "/wallet");
    connect(QNetIO::Wallet::instance(), SIGNAL(credentialsRequired(const QString&, QAuthenticator *)),
        this, SLOT(getCredentials(const QString&, QAuthenticator *)));

    /* initialise settings */
    migrateFromWdesktop();
    settings = new QSettings(this);
    if (isInstalled() && openAtLogin())
        setOpenAtLogin(true);

    /* create system tray icon */
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon;
    trayIcon->setIcon(QIcon(":/wiLink.png"));
    trayMenu = new QMenu;
    QAction *action = trayMenu->addAction(QIcon(":/options.png"), tr("Chat accounts"));
    connect(action, SIGNAL(triggered()), this, SLOT(showAccounts()));
    action = trayMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered()), this, SLOT(quit()));
    trayIcon->setContextMenu(trayMenu);
    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
    QObject::connect(trayIcon, SIGNAL(messageClicked()),
        this, SLOT(messageClicked()));
    trayIcon->show();
#endif
}

Application::~Application()
{
    // destroy tray icon
    if (trayIcon)
    {
#ifdef Q_OS_WIN
        // FIXME : on Windows, deleting the icon crashes the program
        trayIcon->hide();
#else
        delete trayIcon;
        delete trayMenu;
#endif
    }

    // destroy chat windows
    foreach (Chat *chat, chats)
        delete chat;
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

/** Prompt the user for credentials.
 */
void Application::getCredentials(const QString &realm, QAuthenticator *authenticator)
{
    const QString prompt = tr("Enter the username and password for your '%1' account.").arg(realm);

    /* create dialog */
    QDialog *dialog = new QDialog;
    QGridLayout *layout = new QGridLayout;

    layout->addWidget(new QLabel(prompt), 0, 0, 1, 2);

    layout->addWidget(new QLabel(tr("Username")), 1, 0);
    QLineEdit *usernameEdit = new QLineEdit();
    if (!authenticator->user().isEmpty())
        usernameEdit->setEnabled(false);
    layout->addWidget(usernameEdit, 1, 1);

    layout->addWidget(new QLabel(tr("Password")), 2, 0);
    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit, 2, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    dialog->connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    dialog->connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox, 3, 1);

    dialog->setLayout(layout);

    /* prompt user */
    QString username, password;
    username = authenticator->user();
    while (username.isEmpty() || password.isEmpty())
    {
        usernameEdit->setText(username);
        passwordEdit->setText(password);
        if (dialog->exec() != QDialog::Accepted)
        {
            delete dialog;
            return;
        }
        username = usernameEdit->text().trimmed().toLower();
        password = passwordEdit->text().trimmed();
    }
    delete dialog;

    /* try to fix username */
    if (realm == "www.wifirst.net")
        username = username.split("@").first() + "@wifirst.net";
    else if (realm == "www.google.com")
        username = username.split("@").first()+ "@gmail.com";

    authenticator->setUser(username);
    authenticator->setPassword(password);

    /* store credentials */
    QNetIO::Wallet::instance()->setCredentials(realm, username, password);
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
    return settings->value("OpenAtLogin", true).toBool();
}

void Application::setOpenAtLogin(bool run)
{
    const QString appName = qApp->applicationName();
    const QString appPath = executablePath();
#ifdef Q_OS_MAC
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
#endif
#ifdef Q_OS_WIN
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (run)
        registry.setValue(appName, appPath);
    else
        registry.remove(appName);
#endif

    // store preference
    settings->setValue("OpenAtLogin", run);
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
    foreach (Chat *chat, chats)
        delete chat;
    chats.clear();

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
        chat->move(xpos, ypos);
        chat->show();

        chat->open(jid);
        chats << chat;
        xpos += 300;
    }

    /* show chats */
    showChats();
}

void Application::showChats()
{
    foreach (Chat *chat, chats)
    {
        chat->setWindowState(chat->windowState() & ~Qt::WindowMinimized);
        chat->show();
        chat->raise();
        chat->activateWindow();
    }
}

void Application::messageClicked()
{
    emit messageClicked(trayContext);
}

void Application::showMessage(QWidget *context, const QString &title, const QString &message)
{
    if (trayIcon)
    {
        trayContext = context;
        trayIcon->showMessage(title, message);
    }
}

void Application::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        showChats();
}
