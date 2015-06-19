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
#include <QStringList>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <CoreServices/CoreServices.h>

#include "wallet/osx.h"

#if !defined(errSecSuccess)
#define errSecSuccess 0
#endif

OsxWallet::OsxWallet(QObject *parent) : Wallet(parent),
    keychain(NULL)
{
    qDebug("OsxWallet initialised");
}

bool OsxWallet::deleteCredentials(const QString &realm, const QString &user)
{
    const QByteArray realm_data = realm.toUtf8();
    const QByteArray user_data = user.toUtf8();
    SecKeychainItemRef itemRef = NULL;

    /* check whether the entry exists */
    int rc = SecKeychainFindInternetPassword(keychain,
        realm_data.size(), realm_data.constData(),
        0, NULL,
        user_data.size(), user_data.constData(),
        0, NULL,
        0, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
        0, NULL,
        &itemRef);
    if (rc != errSecSuccess)
        return false;

    rc = SecKeychainItemDelete(itemRef);
    CFRelease(itemRef);

    return (rc == errSecSuccess);
}

bool OsxWallet::getCredentials(const QString &realm, QString &user, QString &password)
{
    const QByteArray realm_data = realm.toUtf8();
    const QByteArray user_data = user.toUtf8();
    SecKeychainItemRef itemRef = NULL;

    /* get password */
    int rc = SecKeychainFindInternetPassword(keychain,
        realm_data.size(), realm_data.constData(),
        0, NULL,
        user_data.size(), user_data.constData(),
        0, NULL,
        0, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
        0, NULL,
        &itemRef);
    if (rc != errSecSuccess)
        return false;

    /* get username */
    SecKeychainAttribute attr;
    SecKeychainAttributeList attrList;
    attr.tag = kSecAccountItemAttr;
    attr.length = 0;
    attr.data = NULL;
    attrList.count = 1;
    attrList.attr = &attr;

    UInt32 password_length = 0;
    void *password_data = NULL;

    if (SecKeychainItemCopyContent(itemRef, NULL, &attrList, &password_length, &password_data) != errSecSuccess)
    {
        CFRelease(itemRef);
        return false;
    }

    /* store results */
    user = QString::fromUtf8(QByteArray((char*)attr.data, attr.length));
    password = QString::fromUtf8(QByteArray((char*)password_data, password_length));
    SecKeychainItemFreeContent(&attrList, password_data);
    CFRelease(itemRef);
    return true;
}

bool OsxWallet::getRealms(QStringList &realms)
{
    Q_UNUSED(realms);
    return false;
}

bool OsxWallet::setCredentials(const QString &realm, const QString &user, const QString &password)
{
    const QByteArray realm_data = realm.toUtf8();
    const QByteArray user_data = user.toUtf8();
    const QByteArray password_data = password.toUtf8();

    /* check whether the entry already exists */
    SecKeychainItemRef itemRef = NULL;
    int rc = SecKeychainFindInternetPassword(keychain,
        realm_data.size(), realm_data.constData(),
        0, NULL,
        user_data.size(), user_data.constData(),
        0, NULL,
        0, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
        0, NULL,
        &itemRef);

    if (rc == errSecSuccess)
    {
        // FIXME: we should not update username!
        SecKeychainAttribute attr;
        SecKeychainAttributeList attrList;
        attr.tag = kSecAccountItemAttr;
        attr.length = user_data.size();
        attr.data = (void*)user_data.constData();
        attrList.count = 1;
        attrList.attr = &attr;

        rc = SecKeychainItemModifyAttributesAndData(itemRef, &attrList, password_data.size(), password_data.constData());
        CFRelease(itemRef);
    } else {
        rc = SecKeychainAddInternetPassword(keychain,
            realm_data.size(), realm_data.constData(),
            0, NULL,
            user_data.size(), user_data.constData(),
            0, NULL,
            0, kSecProtocolTypeAny, kSecAuthenticationTypeAny,
            password_data.size(), password_data.constData(), NULL);
    }

    return (rc == errSecSuccess);
}

class OsxWalletPlugin : public QNetIO::WalletPlugin
{
public:
    QNetIO::Wallet *create(const QString &key) {
        if (key.toLower() == QLatin1String("osx"))
            return new OsxWallet;
        return NULL;
    };

    QStringList keys() const {
        return QStringList(QLatin1String("osx"));
    };

    int priority(const QString &key) const {
        Q_UNUSED(key);
        return 1;
    };

};

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_STATIC_PLUGIN2(osx_wallet, OsxWalletPlugin)
#endif
