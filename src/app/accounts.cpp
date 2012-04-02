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

#include "application.h"
#include "accounts.h"
#include "wallet.h"

#include "QXmppUtils.h"

class AccountItem : public ChatModelItem
{
public:
    AccountItem(const QString &jid);
    QString password() const;
    QString type() const;
    QString username() const;

    const QString jid;
};

AccountItem::AccountItem(const QString &jid_)
    : jid(jid_)
{
}

QString AccountItem::password() const
{
    // determine realm
    QString realm;
    const QString domain = jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        realm = QLatin1String("www.wifirst.net");
    else if (domain == QLatin1String("gmail.com"))
        realm = QLatin1String("www.google.com");
    else
        realm = domain;

    QString tmpJid(jid);
    QString password;
    if (QNetIO::Wallet::instance()->getCredentials(realm, tmpJid, password))
        return password;
    else
        return QString();
}

QString AccountItem::type() const
{
    const QString domain = jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("wifirst");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("google");
    else
        return domain;
}

QString AccountItem::username() const
{
    return jid;
}

AccountModel::AccountModel(QObject *parent)
    : ChatModel(parent)
{
    // set additionals role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(UsernameRole, "username");
    names.insert(PasswordRole, "password");
    names.insert(TypeRole, "type");
    setRoleNames(names);

    // load accounts
    _q_reload();
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    AccountItem *item = static_cast<AccountItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == JidRole) {
        return item->jid;
    } else if (role == PasswordRole) {
        return item->password();
    } else if (role == TypeRole) {
        return item->type();
    } else if (role == UsernameRole) {
        return item->username();
    }

    return QVariant();
}

bool AccountModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

void AccountModel::_q_reload()
{
    const QStringList chatJids = wApp->settings()->chatAccounts();
    foreach (const QString &jid, chatJids) {
        addItem(new AccountItem(jid), rootItem);
    }
}

