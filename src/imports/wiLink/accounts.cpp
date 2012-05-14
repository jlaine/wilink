/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#include <QSet>
#include <QSettings>
#include <QStringList>

#include "accounts.h"
#include "wallet.h"

#include "QXmppUtils.h"

static QSet<AccountModel*> globalInstances;

class AccountItem : public ChatModelItem
{
public:
    AccountItem(const QString &type, const QString &realm, const QString &username);
    QString provider() const;

    const QString type;
    const QString realm;
    const QString username;
    QString changedPassword;
};

AccountItem::AccountItem(const QString &type_, const QString &realm_, const QString &username_)
    : type(type_)
    , realm(realm_)
    , username(username_)
{
}

QString AccountItem::provider() const
{
    const QString domain = QXmppUtils::jidToDomain(username);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("wifirst");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("google");
    else
        return domain;
}

AccountModel::AccountModel(QObject *parent)
    : ChatModel(parent)
{
    // set additionals role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(UsernameRole, "username");
    names.insert(PasswordRole, "password");
    names.insert(ProviderRole, "provider");
    names.insert(TypeRole, "type");
    setRoleNames(names);

    // load accounts
    _q_reload();

    globalInstances.insert(this);
}

AccountModel::~AccountModel()
{
    globalInstances.remove(this);
}

void AccountModel::append(const QVariantMap &obj)
{
    const QString type = obj.value("type").toString();
    const QString realm = obj.value("realm").toString();
    const QString username = obj.value("username").toString();
    const QString password = obj.value("password").toString();
    if (type.isEmpty() || realm.isEmpty() || username.isEmpty() || password.isEmpty()) {
        qWarning("Type, realm, username and password are required to add an account");
        return;
    }

    AccountItem *item = new AccountItem(type, realm, username);
    item->changedPassword = password;
    addItem(item, rootItem);
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    AccountItem *item = static_cast<AccountItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == PasswordRole) {
        if (!item->realm.isEmpty()) {
            QString tmpUsername(item->username);
            QString tmpPassword;

            if (QNetIO::Wallet::instance()->getCredentials(item->realm, tmpUsername, tmpPassword))
                return tmpPassword;
        }
    } else if (role == ProviderRole) {
        return item->provider();
    } else if (role == UsernameRole) {
        return item->username;
    } else if (role == TypeRole) {
        return item->type;
    }

    return QVariant();
}

void AccountModel::remove(int index)
{
    removeRows(index, 1);
}

bool AccountModel::submit()
{
    QStringList newAccounts;
    QSettings settings;

    // save passwords for new accounts
    foreach (ChatModelItem *ptr, rootItem->children) {
        AccountItem *item = static_cast<AccountItem*>(ptr);
        if (!item->changedPassword.isEmpty()) {
            setPassword(item->realm, item->username, item->changedPassword);
            item->changedPassword = QString();
        }
        newAccounts << QString("%1:%2:%3").arg(item->type, item->realm, item->username);
    }

#if 0
    // FIXME: remove password for removed accounts
    const QStringList oldJids = settings.value("ChatAccounts").toStringList();
    foreach (const QString &jid, oldJids) {
        if (!newJids.contains(jid)) {
            const QString key = realm(jid);
            qDebug("Removing password for %s (%s)", qPrintable(jid), qPrintable(key));
            QNetIO::Wallet::instance()->deleteCredentials(key, jid);
        }
    }
#endif

    // save accounts
    settings.setValue("Accounts", newAccounts);
    foreach (AccountModel *other, globalInstances) {
        if (other != this)
            other->_q_reload();
    }

    return true;
}

QString AccountModel::getPassword(const QString &jid) const
{
    const QString key = QXmppUtils::jidToDomain(jid);

    if (!key.isEmpty()) {
        QString tmpJid(jid);
        QString tmpPassword;

        if (QNetIO::Wallet::instance()->getCredentials(key, tmpJid, tmpPassword))
            return tmpPassword;
    }
    return QString();
}

bool AccountModel::setPassword(const QString &realm, const QString &username, const QString &password)
{
    if (realm.isEmpty() || username.isEmpty())
        return false;

    qDebug("Setting password for %s (%s)", qPrintable(username), qPrintable(realm));
    return QNetIO::Wallet::instance()->setCredentials(realm, username, password);
}

void AccountModel::_q_reload()
{
    beginResetModel();

    foreach (ChatModelItem *item, rootItem->children)
        delete item;
    rootItem->children.clear();

    QSettings settings;
    if (!settings.contains("Accounts")) {
        QStringList accounts;
        const QStringList chatJids = settings.value("ChatAccounts").toStringList();
        qDebug("migrating accounts");
        foreach (const QString &jid, chatJids) {
            if (QRegExp("^[^@/ ]+@[^@/ ]+$").exactMatch(jid)) {
                const QString domain = QXmppUtils::jidToDomain(jid);
                if (domain == QLatin1String("wifirst.net")) {
                    QString tmpJid(jid);
                    QString tmpPassword;
                    qDebug("Converting wifirst account %s", qPrintable(tmpJid));
                    if (QNetIO::Wallet::instance()->getCredentials("www.wifirst.net", tmpJid, tmpPassword))
                        QNetIO::Wallet::instance()->setCredentials("wifirst.net", tmpJid, tmpPassword);
                    accounts << QString("web:www.wifirst.net:%1").arg(jid);
                } else if (domain == QLatin1String("gmail.com")) {
                    QString tmpJid(jid);
                    QString tmpPassword;
                    if (QNetIO::Wallet::instance()->getCredentials("www.google.com", tmpJid, tmpPassword))
                        QNetIO::Wallet::instance()->setCredentials("gmail.com", tmpJid, tmpPassword);
                    accounts << QString("web:www.google.com:%1").arg(jid);
                }
                accounts << QString("xmpp:%1:%2").arg(domain, jid);
            }
        }
        settings.setValue("Accounts", accounts);
    }

    const QStringList accountStrings = settings.value("Accounts").toStringList();
    foreach (const QString &accountStr, accountStrings) {
        const QStringList bits = accountStr.split(":");
        if (bits.size() == 3) {
            AccountItem *item = new AccountItem(bits[0], bits[1], bits[2]);
            item->parent = rootItem;
            rootItem->children.append(item);
        }
    }

    endResetModel();
}

