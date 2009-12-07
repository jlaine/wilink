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

#include <windows.h>

#include "wireless.h"

typedef DWORD (WINAPI *WlanOpenHandleProto)
    (DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle);
typedef DWORD (WINAPI *WlanCloseHandleProto)(HANDLE hClientHandle, PVOID pReserved);

/* Function pointers */
static HINSTANCE hDLL = NULL;
static WlanOpenHandleProto fpOpenHandle = NULL;
static WlanCloseHandleProto fpCloseHandle = NULL;

WirelessInterface::WirelessInterface(const QString &name)
    : interfaceName(name)
{
    if (!hDLL)
    {
        hDLL = LoadLibrary("wlanapi.dll");
        if (hDLL != NULL)
        {
            fpOpenHandle = (WlanOpenHandleProto)GetProcAddress(hDLL, "WlanOpenHandle");
            fpCloseHandle = (WlanCloseHandleProto)GetProcAddress(hDLL, "WlanCloseHandle");
        } else {
            qWarning("Could not open wlanapi.dll");
        }
    }
}

bool WirelessInterface::isValid() const
{
    return false;
}

QList<WirelessNetwork> WirelessInterface::networks()
{
    return QList<WirelessNetwork>();
}

