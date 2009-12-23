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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "application.h"

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

bool Application::setRunAtLogin(bool run)
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
        QByteArray regValue = QByteArray((const char*)appPath.utf16(), (appPath.size() + 1) * 2);
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

