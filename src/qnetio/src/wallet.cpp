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

#include <QAuthenticator>
#include <QCoreApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QNetworkReply>
#include <QPluginLoader>
#include <QSslError>
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
 *
 * Project page: http://opensource.bolloretelecom.eu/projects/qnetio/
 */

/* native wallets */
#ifdef USE_OSX_KEYCHAIN
Q_IMPORT_PLUGIN(osx_wallet)
#endif
#ifdef USE_WINDOWS_KEYRING
Q_IMPORT_PLUGIN(windows_wallet)
#endif

/* fallback wallet */
Q_IMPORT_PLUGIN(dummy_wallet)

using namespace QNetIO;

static Wallet *active = 0;
static QString walletDataPath;

static bool backend_less_than(const Wallet::Backend &b1, const Wallet::Backend &b2)
{
    return b1.priority > b2.priority;
}

QObjectList QNetIO::pluginInstances(const QString &type)
{
    QObjectList plugins;

    // static plugins
    foreach (QObject *plugin, QPluginLoader::staticInstances())
        if (plugin && !plugins.contains(plugin))
            plugins << plugin;

    // dynamic plugins
    foreach (QString dirName, qApp->libraryPaths())
    {
        QDir path(dirName + "/" + type);
        foreach (QString pname, path.entryList(QDir::Files))
        {
            QPluginLoader loader(path.absoluteFilePath(pname));
            QObject *plugin = loader.instance();
            if (plugin && !plugins.contains(plugin))
                plugins << plugin;
        }
    }
    return plugins;
}

/** Construct a Wallet with the given parent.
 */
Wallet::Wallet(QObject *parent) : QObject(parent)
{
    if (!active)
        active = this;
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

/** Returns the list of available Wallet backends.
 */
QList<Wallet::Backend> Wallet::backends()
{
    QList<Wallet::Backend> backends;
    foreach (QObject *obj, pluginInstances("wallet"))
    {
        WalletPlugin *plugin = qobject_cast<WalletPlugin *>(obj);
        if (!plugin)
            continue;

        foreach (const QString &key, plugin->keys())
        {
            Backend backend;
            backend.plugin = plugin;
            backend.key = key;
            backend.priority = plugin->priority(key);
            backends << backend;
        }
    }
    qSort(backends.begin(), backends.end(), backend_less_than);
    return backends;
}

/** Creates a wallet using the specified backend.
 *
 * @param key
 */
Wallet *Wallet::create(const QString &key)
{
    foreach (const Wallet::Backend &backend, backends())
    {
        if (!key.isEmpty() && backend.key != key)
            continue;
        Wallet *wallet = backend.plugin->create(backend.key);
        if (wallet)
            return wallet;
    }
    return 0;
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
    if (!active)
        create();
    return active;
}

/** Invalidate the wallet data, causing a re-read on next access.
 *
 * NOTE: only applies to cache-based wallets.
 */
void Wallet::invalidate()
{
}

/** Return the current user's credentials for a site.
 *
 * @param realm
 * @param authenticator
 */
void Wallet::onAuthenticationRequired(const QString &realm, QAuthenticator *authenticator)
{
    QString user, password;
    QStringList bits = realm.split(".");
    while (bits.size() > 2)
        bits.removeFirst();
    QString domain = bits.join(".");

    qDebug() << "Wallet authentication required for" << realm;
    if ((getCredentials(realm, user, password) || getCredentials(domain, user, password)) &&
        (authenticator->user() != user || authenticator->password() != password))
    {
        authenticator->setUser(user);
        authenticator->setPassword(password);
        return;
    }
    qWarning() << "Could not find password for" << realm;
    emit credentialsRequired(realm, authenticator);
}

/** Returns the current user's credentials for a network reply.
 *
 * @param reply
 * @param authenticator
 */
void Wallet::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    onAuthenticationRequired(reply->url().host(), authenticator);
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

