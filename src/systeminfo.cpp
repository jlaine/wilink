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

#include <QDesktopServices>
#include <QDir>
#include <QProcess>

#include "systeminfo.h"

QString SystemInfo::displayName(SystemInfo::StorageLocation type)
{
    if (type == SystemInfo::DownloadsLocation)
        return QObject::tr("Downloads");
    else if (type == SystemInfo::SharesLocation)
        return QObject::tr("Shares");
    return QString();
}

QString SystemInfo::storageLocation(SystemInfo::StorageLocation type)
{
    if (type == SystemInfo::DownloadsLocation)
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
    else if (type == SystemInfo::SharesLocation)
    {
        return QDir::home().filePath("Public");
    }
    return QString();
}

QString SystemInfo::osName()
{
#ifdef Q_OS_LINUX
    return QString::fromLatin1("Linux");
#endif
#ifdef Q_OS_MAC
    return QString::fromLatin1("Mac OS");
#endif
#ifdef Q_OS_WIN
    return QString::fromLatin1("Windows");
#endif
    return QString::fromLatin1("Unknown");
}

QString SystemInfo::osType()
{
#if defined(Q_OS_WIN)
    return QString::fromLatin1("win32");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("mac");
#elif defined(Q_OS_LINUX)
    return QString::fromLatin1("linux");
#else
    return QString();
#endif
}

QString SystemInfo::osVersion()
{
#ifdef Q_OS_LINUX
    QProcess process;
    process.start(QString("uname"), QStringList(QString("-r")), QIODevice::ReadOnly);
    process.waitForFinished();
    return QString::fromLocal8Bit(process.readAllStandardOutput());
#endif
#ifdef Q_OS_MAC
    switch (QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_10_4:
        return QString::fromLatin1("10.4");
    case QSysInfo::MV_10_5:
        return QString::fromLatin1("10.5");
    case QSysInfo::MV_10_6:
        return QString::fromLatin1("10.6");
    }
#endif
#ifdef Q_OS_WIN
    switch (QSysInfo::WindowsVersion)
    {
    case QSysInfo::WV_XP:
        return QString::fromLatin1("XP");
    case QSysInfo::WV_2003:
        return QString::fromLatin1("2003");
    case QSysInfo::WV_VISTA:
        return QString::fromLatin1("Vista");
    case QSysInfo::WV_WINDOWS7:
        return QString::fromLatin1("7");
    }
#endif
    return QString();
}


