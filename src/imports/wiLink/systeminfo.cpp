/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#include <QDir>
#include <QProcess>

#include "systeminfo.h"

QString SystemInfo::osName()
{
#if defined(Q_OS_ANDROID)
    return QString::fromLatin1("Android");
#elif defined(Q_OS_LINUX)
    return QString::fromLatin1("Linux");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("Mac OS");
#elif defined(Q_OS_WIN)
    return QString::fromLatin1("Windows");
#else
    return QString::fromLatin1("Unknown");
#endif
}

QString SystemInfo::osType()
{
#if defined(Q_OS_ANDROID)
    return QString::fromLatin1("android");
#elif defined(Q_OS_LINUX)
    return QString::fromLatin1("linux");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("mac");
#elif defined(Q_OS_WIN)
    return QString::fromLatin1("win32");
#else
    return QString();
#endif
}

QString SystemInfo::osVersion()
{
    return QSysInfo::productVersion();
}
