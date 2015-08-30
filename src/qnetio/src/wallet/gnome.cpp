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

#include <QDebug>
#include <QCoreApplication>
#include <QStringList>

#include <gnome-keyring.h>

#include "wallet/gnome.h"

GnomeWallet::GnomeWallet(QObject *parent) : Wallet(parent)
{
    qDebug("GnomeWallet initialised");
    g_set_application_name(qApp->applicationName().toLatin1().constData());
}

bool GnomeWallet::deleteCredentials(const QString &realm, const QString &user)
{
    GnomeKeyringResult res;

    if (user.isEmpty()) {
        res = gnome_keyring_delete_password_sync(
            GNOME_KEYRING_NETWORK_PASSWORD,
            "server", realm.toLatin1().constData(),
            NULL);
    } else {
        res = gnome_keyring_delete_password_sync(
            GNOME_KEYRING_NETWORK_PASSWORD,
            "server", realm.toLatin1().constData(),
            "user", user.toLatin1().constData(),
            NULL);
    }

    return (res == GNOME_KEYRING_RESULT_OK);
}

bool GnomeWallet::getCredentials(const QString &realm, QString &user, QString &password)
{
    GnomeKeyringResult res;
    GList *results = NULL;
    const QByteArray realm_data = realm.toLatin1();
    const QByteArray user_data = user.toLatin1();

    res = gnome_keyring_find_network_password_sync(user_data.isEmpty() ? NULL : user_data.constData(), NULL, realm_data.constData(), NULL, NULL, NULL, 0, &results);
    if (res != GNOME_KEYRING_RESULT_OK)
        return false;

    GnomeKeyringNetworkPasswordData *data = (GnomeKeyringNetworkPasswordData *)g_list_nth_data(results, 0);
    user = QString::fromAscii(data->user);
    password = QString::fromAscii(data->password);
    gnome_keyring_network_password_list_free(results);
    return true;
}

bool GnomeWallet::getRealms(QStringList &realms)
{
    GnomeKeyringResult res;
    GList *results = NULL;

    res = gnome_keyring_find_network_password_sync(NULL, NULL, NULL, NULL, NULL, NULL, 0, &results);
    if (res != GNOME_KEYRING_RESULT_OK)
        return false;

    GnomeKeyringNetworkPasswordData *data = (GnomeKeyringNetworkPasswordData *)g_list_nth_data(results, 0);
    QString realm = QString::fromAscii(data->server);
    realms << realm;

    gnome_keyring_network_password_list_free(results);
    return true;
}

bool GnomeWallet::setCredentials(const QString &realm, const QString &user, const QString &password)
{
    const QByteArray realm_data = realm.toLatin1();
    const QByteArray user_data = user.toLatin1();
    const QByteArray password_data = password.toLatin1();

    GnomeKeyringResult res = gnome_keyring_store_password_sync(
        GNOME_KEYRING_NETWORK_PASSWORD,
        NULL,
        realm_data.constData(),
        password_data.constData(),
        "server", realm_data.constData(),
        "user", user_data.constData(),
        NULL);
    return (res == GNOME_KEYRING_RESULT_OK);
}

class GnomeWalletPlugin : public QNetIO::WalletPlugin
{
public:
    QNetIO::Wallet *create(const QString &key) {
        if (key.toLower() == QLatin1String("gnome"))
            return new GnomeWallet;
        return NULL;
    };

    QStringList keys() const {
        return QStringList(QLatin1String("gnome"));
    };

    int priority(const QString &key) const {
        Q_UNUSED(key);
        const char *session = getenv("DESKTOP_SESSION");
        if (session && !strcmp(session, "gnome"))
            return 1;
        else
            return 0;
    };
};

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(gnome_wallet, GnomeWalletPlugin)
#endif
