/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibrary>

#include <windows.h>
#include <wincrypt.h>

#include "wallet/windows.h"

typedef bool (CALLBACK* LPFNDLL_ENCRYPT)(DATA_BLOB*,LPCWSTR, DATA_BLOB*, PVOID, CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);
typedef bool (CALLBACK* LPFNDLL_DECRYPT)(DATA_BLOB*, LPWSTR*, DATA_BLOB*, PVOID, CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);

/* Set up encryption functions */
static LPFNDLL_ENCRYPT fpEncrypt = NULL;
static LPFNDLL_DECRYPT fpDecrypt = NULL;

WindowsWallet::WindowsWallet(QObject *parent) : HashWallet(parent)
{
    qDebug("WindowsWallet initialised");
    QLibrary lib(QLatin1String("crypt32.dll"));
    if (lib.load()) {
        fpDecrypt = LPFNDLL_DECRYPT(lib.resolve("CryptUnprotectData"));
        fpEncrypt = LPFNDLL_ENCRYPT(lib.resolve("CryptProtectData"));
    }
}

WindowsWallet::~WindowsWallet()
{
}

bool WindowsWallet::read()
{
    const QString filePath = dataPath() + ".windows";

    if (!fpDecrypt)
    {
        qWarning("Cannot find Windows decryption function");
        return false;
    }

    /* read from file */
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Could not read" << filePath;
        return false;
    }
    QByteArray encrypted = file.readAll();
    file.close();

    /* decrypt data */
    DATA_BLOB in, out;
    in.pbData = (BYTE*)encrypted.data();
    in.cbData = encrypted.size();
    if (!fpDecrypt(&in, NULL, NULL, NULL, NULL, 0, &out))
    {
        qWarning("Windows decryption failed");
        return false;
    }
    QByteArray byteArray((const char*)out.pbData, out.cbData);
    QDataStream stream(&byteArray, QIODevice::ReadOnly);
    stream >> creds;
    return true;
}

bool WindowsWallet::write()
{
    const QString filePath = dataPath() + ".windows";

    if (!fpDecrypt)
    {
        qWarning("Cannot find Windows encryption function");
        return false;
    }

    /* encrypt data */
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << creds;

    DATA_BLOB in, out;
    in.pbData = (BYTE*)byteArray.data();
    in.cbData = byteArray.size();
    if (!fpEncrypt(&in, L"QNetIO Secure Password", NULL, NULL, NULL, 0, &out))
    {
        qWarning("Windows encryption failed");
        return false;
    }

    /* write to file */
    QByteArray encrypted((const char*)out.pbData, out.cbData);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write" << filePath;
        return false;
    }
    file.write(encrypted);
    file.close();

    return true;
}
