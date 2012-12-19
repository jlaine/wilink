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

#ifndef __QNETIO_WALLET_H__
#define __QNETIO_WALLET_H__

#include <QObject>
#include <QHash>
#include <QList>
#include <QtPlugin>

class QAuthenticator;
class QNetworkReply;
class QSslError;

namespace QNetIO {

QObjectList pluginInstances(const QString &type);

class WalletPlugin;

/** @defgroup Wallet Wallet classes
 */

/** Wallet is a singleton class which provides access to credentials.
 *
 * @ingroup Wallet
 */
class Wallet : public QObject
{
    Q_OBJECT

public:
    class Backend
    {
    public:
        WalletPlugin *plugin;
        QString key;
        int priority;
    };

    Wallet(QObject *parent = 0);
    ~Wallet();

    static QList<Wallet::Backend> backends();
    static Wallet *create(const QString &key = QString());
    static QString dataPath();
    static void setDataPath(const QString &path);
    static Wallet *instance();

    /** Removes the credentials for the given realm.
     */
    virtual bool deleteCredentials(const QString &realm, const QString &user) = 0;

    /** Returns the user and password for a given realm.
     *
     * If a user is specified, it will be preferred.
     */
    virtual bool getCredentials(const QString &realm, QString &user, QString &password) = 0;

    /** Lists the available realms.
     */
    virtual bool getRealms(QStringList &realms);

    /** Sets the credentials for the given realm.
     */
    virtual bool setCredentials(const QString &realm, const QString &user, const QString &password) = 0;
    virtual void invalidate();

signals:
    /** This signal is emitted when no credentials could be found
     *  for a given realm.
     */
    void credentialsRequired(const QString &realm, QAuthenticator *authenticator);

public slots:
    void onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void onAuthenticationRequired(const QString &hostname, QAuthenticator *authenticator);
};

class HashWallet : public Wallet
{
    Q_OBJECT

public:
    HashWallet(QObject *parent=NULL);

    virtual bool deleteCredentials(const QString &realm, const QString &user);
    virtual bool getCredentials(const QString &realm, QString &user, QString &password);
    virtual bool getRealms(QStringList &realms);
    virtual bool setCredentials(const QString &realm, const QString &user, const QString &password);
    virtual void invalidate();

protected:
    virtual bool read() = 0;
    virtual bool write() = 0;

    QHash<QString, QPair<QString, QString> > creds;

private:
    void ensureLoaded();
    bool loaded;
};

class WalletPluginInterface
{
public:
    virtual ~WalletPluginInterface() {};

    virtual Wallet *create(const QString &key) = 0;
    virtual QStringList keys() const = 0;
    virtual int priority(const QString &key) const {
        Q_UNUSED(key);
        return 0;
    };
};

}

Q_DECLARE_INTERFACE(QNetIO::WalletPluginInterface, "eu.bolloretelecom.WalletPlugin/1.0")

namespace QNetIO {

class WalletPlugin : public QObject, public WalletPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QNetIO::WalletPluginInterface)
};

}

#endif
