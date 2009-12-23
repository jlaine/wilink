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

#include <signal.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QProcess>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "qnetio/wallet.h"
#include "trayicon.h"

static void signal_handler(int sig)
{
    qApp->quit();
}

class Package
{
public:
    static bool isInstalled();
    static bool setRunAtLogin(bool run);
};

bool Package::isInstalled()
{
    const QString appPath = qApp->applicationFilePath();
    return false;
}

bool Package::setRunAtLogin(bool run)
{
    const QString appName = qApp->applicationName();
#ifdef Q_OS_MAC
    QDir bundleDir(QString("/Applications/%1.app").arg(appName));
    if (!bundleDir.exists())
        return false;

    QString script = run ?
        QString("tell application \"System Events\"\n"
            "\tmake login item at end with properties {path:\"%1\"}\n"
            "end tell\n").arg(bundleDir.absolutePath()) :
        QString("tell application \"System Events\"\n"
            "\tdelete login item \"%1\"\n"
            "end tell\n").arg(appName);
    QProcess process;
    process.start("osascript");
    process.write(script.toAscii());
    process.closeWriteChannel();
    process.waitForFinished();
    return true;
#endif
#ifdef Q_OS_WIN
    HKEY handle;
    DWORD result = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE, &handle);
    if (result != ERROR_SUCCESS)
    {
        qWarning("Could not open registry key");
        return false;
    }
    if (run)
    {
        const QString appPath = qApp->applicationFilePath();
        QByteArray regValue = QByteArray((const char*)appPath.utf16(), (appPath.length() + 1) * 2);
        result = RegSetValueExW(handle, reinterpret_cast<const wchar_t *>(appName.utf16()), 0, REG_SZ,
            reinterpret_cast<const unsigned char*>(regValue.constData()), regValue.size());
    } else {
        result = RegDeleteValueW(handle, reinterpret_cast<const wchar_t *>(appName.utf16()));
    }
    RegCloseKey(handle);
    return (result == ERROR_SUCCESS);
#endif
    return false;
}

int main(int argc, char *argv[])
{
    /* Create application */
    QApplication app(argc, argv);
    app.setApplicationName("wDesktop");
    app.setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    app.setWindowIcon(QIcon(":/wDesktop.png"));
#endif

    /* Make sure application runs at login */
    if (Package::isInstalled())
        Package::setRunAtLogin(true);

    /* Load translations */
    QTranslator translator;
    translator.load(QString(":/%1.qm").arg(QLocale::system().name().split("_").first()));
    app.installTranslator(&translator);

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Setup system tray icon */
    TrayIcon trayIcon;
    trayIcon.show();

    /* Run application */
    return app.exec();
}

