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

#include <QCoreApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QStringList>

#include "wallet.h"

/*! \mainpage
 *
 * QNetIO is a lightweight, cross-platform library which provides support for:
 *
 * - accessing asynchronous virtual file systems
 * - storing credentials using platform-specific methods

 * \sa FileSystem
 * \sa Wallet
 */

#if defined(USE_OSX_KEYCHAIN)
#include "wallet/osx.h"
#elif defined(USE_WINDOWS_KEYRING)
#include "wallet/windows.h"
#else
#include "wallet/dummy.h"
#endif

using namespace QNetIO;

static Wallet *active = 0;
static QString walletDataPath;

/** Construct a Wallet with the given parent.
 */
Wallet::Wallet(QObject *parent) : QObject(parent)
{
}

Wallet::~Wallet()
{
    if (active == this)
        active = 0;
}

/** Return the current user's credentials for a site.
 *
 * @param realms
 */
bool Wallet::getRealms(QStringList &realms)
{
    Q_UNUSED(realms);
    qWarning() << "Could not get realms, not implemented";
    return false;
}

/** Returns the path for wallet data.
 */
QString Wallet::dataPath()
{
    if (!walletDataPath.isEmpty())
        return walletDataPath;
    else
        return QDir::home().filePath(qApp->applicationName());
}

/** Sets the path for wallet data.
 *
 * @param dataPath
 */
void Wallet::setDataPath(const QString &dataPath)
{
    walletDataPath = dataPath;
}

/** Return the currently active Wallet.
 */
Wallet *Wallet::instance()
{
    if (!active) {
#if defined(USE_OSX_KEYCHAIN)
        active = new OsxWallet();
#elif defined(USE_WINDOWS_KEYRING)
        active = new WindowsWallet();
#else
        active = new DummyWallet();
#endif
    }
    return active;
}

/** Invalidate the wallet data, causing a re-read on next access.
 *
 * NOTE: only applies to cache-based wallets.
 */
void Wallet::invalidate()
{
}

HashWallet::HashWallet(QObject *parent) : Wallet(parent), loaded(false)
{
}

void HashWallet::invalidate()
{
    creds.clear();
    loaded = false;
}

bool HashWallet::deleteCredentials(const QString &realm, const QString &user)
{
    if (!creds.contains(realm))
        return false;
    if (!user.isEmpty() && creds.value(realm).first != user)
        return false;

    QHash<QString, QPair<QString, QString> > oldCreds = creds;
    creds.remove(realm);
    if (!write())
    {
        creds = oldCreds;
        return false;
    }

    return true;
}

bool HashWallet::getCredentials(const QString &realm, QString &user, QString &password)
{
    ensureLoaded();

    if (!creds.contains(realm))
        return false;
    if (!user.isEmpty() && creds.value(realm).first != user)
        return false;

    QPair<QString, QString> entry = creds[realm];
    user = entry.first;
    password = entry.second;
    return true;
}

bool HashWallet::getRealms(QStringList &realms)
{
    ensureLoaded();

    realms = creds.keys();
    return true;
}

void HashWallet::ensureLoaded()
{
    if (!loaded && read())
        loaded = true;
}

bool HashWallet::setCredentials(const QString &realm, const QString &user, const QString &password)
{
    ensureLoaded();

    QHash<QString, QPair<QString, QString> > oldCreds = creds;
    creds[realm] = qMakePair(user, password);

    if (!write())
    {
        creds = oldCreds;
        return false;
    }
    return true;
}

