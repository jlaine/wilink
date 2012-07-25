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
#include "client.h"
#include "wallet.h"

#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

static QSet<AccountModel*> globalInstances;

class AccountItem : public ChatModelItem
{
public:
    AccountItem(const QString &type, const QString &realm, const QString &username);

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

class AccountModelPrivate
{
public:
    QSet<QPair<QString,QString> > removedCredentials;
};

AccountModel::AccountModel(QObject *parent)
    : ChatModel(parent)
    , d(new AccountModelPrivate)
{
    // set additionals role names
    QHash<int, QByteArray> names;
    names.insert(UsernameRole, "username");
    names.insert(PasswordRole, "password");
    names.insert(RealmRole, "realm");
    names.insert(TypeRole, "type");
    setRoleNames(names);

    // load accounts
    _q_reload();

    globalInstances.insert(this);
}

AccountModel::~AccountModel()
{
    globalInstances.remove(this);
    delete d;
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

    d->removedCredentials.remove(qMakePair(realm, username));
}

ChatClient* AccountModel::clientForJid(const QString &jid)
{
    const QString bareJid = QXmppUtils::jidToBareJid(jid);
    QList<ChatClient*> clients = ChatClient::instances();

    // look for own jid or domain
    foreach (ChatClient *client, clients) {
        if (QXmppUtils::jidToBareJid(client->jid()) == bareJid
            || QXmppUtils::jidToDomain(client->jid()) == QXmppUtils::jidToDomain(bareJid))
            return client;
    }

    // look for jid in contacts
    foreach (ChatClient *client, clients) {
        if (client->rosterManager()->getRosterBareJids().contains(bareJid))
            return client;
    }

    // return first client
    return clients.isEmpty() ? 0 : clients.first();
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
    } else if (role == UsernameRole) {
        return item->username;
    } else if (role == RealmRole) {
        return item->realm;
    } else if (role == TypeRole) {
        return item->type;
    }

    return QVariant();
}

void AccountModel::remove(int row)
{
    const QModelIndex idx = index(row, 0);
    if (idx.isValid()) {
        AccountItem *item = static_cast<AccountItem*>(idx.internalPointer());
        d->removedCredentials.insert(qMakePair(item->realm, item->username));
        removeRows(row, 1);
    }
}

void AccountModel::setProperty(int row, const QString &property, const QVariant &value)
{
    const QModelIndex idx = index(row, 0);
    if (idx.isValid() && property == "password") {
        AccountItem *item = static_cast<AccountItem*>(idx.internalPointer());
        item->changedPassword = value.toString();
    }
}

bool AccountModel::submit()
{
    QStringList newAccounts;
    QSettings settings;

    // save passwords for new accounts
    foreach (ChatModelItem *ptr, rootItem->children) {
        AccountItem *item = static_cast<AccountItem*>(ptr);
        if (!item->changedPassword.isEmpty()) {
            qDebug("Setting password for %s (%s)", qPrintable(item->username), qPrintable(item->realm));
            QNetIO::Wallet::instance()->setCredentials(item->realm, item->username, item->changedPassword);
            item->changedPassword = QString();
        }
        newAccounts << QString("%1:%2:%3").arg(item->type, item->realm, item->username);
    }

    // remove password for removed accounts
    QSet<QPair<QString,QString> >::const_iterator it;
    for (it = d->removedCredentials.constBegin(); it != d->removedCredentials.constEnd(); ++it) {
        qDebug("Removing password for %s (%s)", qPrintable(it->second), qPrintable(it->first));
        QNetIO::Wallet::instance()->deleteCredentials(it->first, it->second);
    }
    d->removedCredentials.clear();

    // save accounts
    settings.setValue("Accounts", newAccounts);
    foreach (AccountModel *other, globalInstances) {
        if (other != this)
            other->_q_reload();
    }

    return true;
}

void AccountModel::_q_reload()
{
    beginResetModel();

    // clear data
    foreach (ChatModelItem *item, rootItem->children)
        delete item;
    rootItem->children.clear();
    d->removedCredentials.clear();

    QSettings settings;

    // migrate from wiLink < 2.3
    if (!settings.contains("Accounts") && settings.contains("ChatAccounts")) {
        QStringList accounts;
        const QStringList chatJids = settings.value("ChatAccounts").toStringList();
        foreach (const QString &jid, chatJids) {
            if (QRegExp("^[^@/ ]+@[^@/ ]+$").exactMatch(jid)) {
                const QString domain = QXmppUtils::jidToDomain(jid);
                if (domain == QLatin1String("wifirst.net")) {
                    QString tmpJid(jid);
                    QString tmpPassword;
                    qDebug("Converting wifirst account %s", qPrintable(tmpJid));
                    accounts << QString("web:www.wifirst.net:%1").arg(jid);
                } else if (domain == QLatin1String("gmail.com")) {
                    QString tmpJid(jid);
                    QString tmpPassword;
                    accounts << QString("web:www.google.com:%1").arg(jid);
                } else {
                    accounts << QString("xmpp:%1:%2").arg(domain, jid);
                }
            }
        }
        settings.setValue("Accounts", accounts);
        settings.remove("ChatAccounts");
    }

    const QStringList accountStrings = settings.value("Accounts").toStringList();
    foreach (const QString &accountStr, accountStrings) {
        const QStringList bits = accountStr.split(":");
        if (bits.size() == 3) {
            // migrate from wiLink < 2.4
            if (bits[0] == "xmpp" && (bits[1] == "wifirst.net" || bits[1] == "gmail.com")) {
                QNetIO::Wallet::instance()->deleteCredentials(bits[1], bits[2]);
                continue;
            }

            AccountItem *item = new AccountItem(bits[0], bits[1], bits[2]);
            item->parent = rootItem;
            rootItem->children.append(item);
        }
    }

    endResetModel();
}

