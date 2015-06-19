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

#include <cstdlib>

#include <QCoreApplication>
#include <QMap>
#include <QStringList>
#include <kwallet.h>

#include "wallet/kde.h"

KdeWallet::KdeWallet(QObject *parent) : Wallet(parent), currentWallet(NULL)
{
    QString foo = KWallet::Wallet::LocalWallet();
    if (!(currentWallet = KWallet::Wallet::openWallet(foo, (WId) 0))) // FIXME: set proper WId
        return;
    if (!currentWallet->hasFolder(qApp->applicationName()))
        currentWallet->createFolder(qApp->applicationName());
    currentWallet->setFolder(qApp->applicationName());
    // FIXME: check it works properly
}

bool KdeWallet::deleteCredentials(const QString &realm, const QString &user)
{
    if(!currentWallet || currentWallet->removeEntry(realm))
        return false;
    return true;
}

bool KdeWallet::getCredentials(const QString &realm, QString &user, QString &password)
{
    QMap<QString, QString> value;
    if(currentWallet && currentWallet->hasEntry(realm) && !currentWallet->readMap(realm, value)) {
        user = value["user"];
        password = value["password"];
        return true;
    }
    return false;
}

bool KdeWallet::getRealms(QStringList &realms)
{
    if(currentWallet) {
        realms = currentWallet->entryList();
        if(!realms.isEmpty())
            return true;
    }
    return false;
}

bool KdeWallet::setCredentials(const QString &realm, const QString &user, const QString &password)
{
    QMap<QString, QString> value;
    value["user"] = user;
    value["password"] = password;
    if(currentWallet && !currentWallet->writeMap(realm, value))
        return true;
    return false;
}

class KdeWalletPlugin : public QNetIO::WalletPlugin
{
public:
    QNetIO::Wallet *create(const QString &key) {
        if (key.toLower() == QLatin1String("kde"))
            return new KdeWallet;
        return NULL;
    };

    QStringList keys() const {
        return QStringList(QLatin1String("kde"));
    };

    int priority(const QString &key) const {
        Q_UNUSED(key);
        const char *session = getenv("DESKTOP_SESSION");
        if (session && !strcmp(session, "kde"))
            return 1;
        else
            return 0;
    };
};

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(kde_wallet, KdeWalletPlugin)
#endif
