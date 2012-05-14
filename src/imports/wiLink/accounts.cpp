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
    AccountItem(const QString &type, const QString &username);
    QString provider() const;

    const QString type;
    const QString username;
    QString changedPassword;
};

static QString realm(const QString &jid)
{
    const QString domain = QXmppUtils::jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("www.wifirst.net");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("www.google.com");
    else
        return domain;
}

AccountItem::AccountItem(const QString &type_, const QString &username_)
    : type(type_)
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
    const QString username = obj.value("username").toString();
    const QString password = obj.value("password").toString();
    const QString type = obj.value("type").toString();

    if (type != "chat") {
        qWarning("Invalid account type specified");
        return;
    }

    if (username.isEmpty() || password.isEmpty()) {
        qWarning("Username and password are required to add an account");
        return;
    }

    AccountItem *item = new AccountItem(type, username);
    item->changedPassword = password;
    addItem(item, rootItem);
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    AccountItem *item = static_cast<AccountItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == JidRole) {
        return item->username;
    } else if (role == PasswordRole) {
        const QString key = realm(item->username);
        if (!key.isEmpty()) {
            QString tmpUsername(item->username);
            QString tmpPassword;

            if (QNetIO::Wallet::instance()->getCredentials(key, tmpUsername, tmpPassword))
                return tmpPassword;
        }
    } else if (role == ProviderRole) {
        return item->provider();
    } else if (role == UsernameRole) {
        return item->username;
    } else if (role == UsernameRole) {
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
    QStringList newJids;
    QSettings settings;

    // save passwords for new accounts
    foreach (ChatModelItem *ptr, rootItem->children) {
        AccountItem *item = static_cast<AccountItem*>(ptr);
        if (item->type == "chat") {
            if (!item->changedPassword.isEmpty()) {
                setPassword(item->username, item->changedPassword);
                item->changedPassword = QString();
            }
            newJids << item->username;
        }
    }

    // remove password for removed accounts
    const QStringList oldJids = settings.value("ChatAccounts").toStringList();
    foreach (const QString &jid, oldJids) {
        if (!newJids.contains(jid)) {
            const QString key = realm(jid);
            qDebug("Removing password for %s (%s)", qPrintable(jid), qPrintable(key));
            QNetIO::Wallet::instance()->deleteCredentials(key);
        }
    }

    // save accounts
    settings.setValue("ChatAccounts", newJids);
    foreach (AccountModel *other, globalInstances) {
        if (other != this)
            other->_q_reload();
    }

    return true;
}

QString AccountModel::getPassword(const QString &jid) const
{
    const QString key = realm(jid);

    if (!key.isEmpty()) {
        QString tmpJid(jid);
        QString tmpPassword;

        if (QNetIO::Wallet::instance()->getCredentials(key, tmpJid, tmpPassword))
            return tmpPassword;
    }
    return QString();
}

bool AccountModel::setPassword(const QString &jid, const QString &password)
{
    const QString key = realm(jid);
    if (key.isEmpty())
        return false;

    qDebug("Setting password for %s (%s)", qPrintable(jid), qPrintable(key));
    return QNetIO::Wallet::instance()->setCredentials(key, jid, password);
}

void AccountModel::_q_reload()
{
    beginResetModel();

    foreach (ChatModelItem *item, rootItem->children)
        delete item;
    rootItem->children.clear();

    const QStringList chatJids = QSettings().value("ChatAccounts").toStringList();
    foreach (const QString &jid, chatJids) {
        if (QRegExp("^[^@/ ]+@[^@/ ]+$").exactMatch(jid)) {
            AccountItem *item = new AccountItem("chat", jid);
            item->parent = rootItem;
            rootItem->children.append(item);
        }
    }

    endResetModel();
}

