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

#include <QDesktopServices>
#include <QDir>
#include <QProcess>

#include "systeminfo.h"

QString SystemInfo::displayName(SystemInfo::StorageLocation type)
{
    if (type == SystemInfo::DownloadsLocation)
        return QObject::tr("Downloads");
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
    return QString();
}

QString SystemInfo::osName()
{
#if defined(Q_OS_LINUX)
    return QString::fromLatin1("Linux");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("Mac OS");
#elif defined(Q_OS_SYMBIAN)
    return QString::fromLatin1("Symbian");
#elif defined(Q_OS_WIN)
    return QString::fromLatin1("Windows");
#else
    return QString::fromLatin1("Unknown");
#endif
}

QString SystemInfo::osType()
{
#if defined(Q_OS_LINUX)
    return QString::fromLatin1("linux");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("mac");
#elif defined(Q_OS_SYMBIAN)
    return QString::fromLatin1("symbian");
#elif defined(Q_OS_WIN)
    return QString::fromLatin1("win32");
#else
    return QString();
#endif
}

QString SystemInfo::osVersion()
{
#if defined(Q_OS_LINUX)
    QProcess process;
    process.start(QString("uname"), QStringList(QString("-r")), QIODevice::ReadOnly);
    process.waitForFinished();
    return QString::fromLocal8Bit(process.readAllStandardOutput());
#elif defined(Q_OS_MAC)
    switch (QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_10_4:
        return QString::fromLatin1("10.4");
    case QSysInfo::MV_10_5:
        return QString::fromLatin1("10.5");
    case QSysInfo::MV_10_6:
        return QString::fromLatin1("10.6");
    default:
        return QString();
    }
#elif defined(Q_OS_SYMBIAN)
    switch (QSysInfo::symbianVersion())
    {
    case QSysInfo::SV_SF_1:
        return QString::fromLatin1("1");
    case QSysInfo::SV_SF_2:
        return QString::fromLatin1("2");
    case QSysInfo::SV_SF_3:
        return QString::fromLatin1("3");
    case QSysInfo::SV_SF_4:
        return QString::fromLatin1("4");
    default:
        return QString();
    }
#elif defined(Q_OS_WIN)
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
    default:
        return QString();
    }
#else
    return QString();
#endif
}


