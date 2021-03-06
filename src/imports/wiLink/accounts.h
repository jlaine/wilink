/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#ifndef __ACCOUNTS_H__
#define __ACCOUNTS_H__

#include "model.h"

class AccountModelPrivate;
class ChatClient;

class AccountModel : public ChatModel
{
    Q_OBJECT

public:
    AccountModel(QObject *parent = 0);
    ~AccountModel();

    /// \cond
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    /// \endcond

public slots:
    ChatClient *clientForJid(const QString &jid = QString());
    void reload();

    // QML ListModel
    void append(const QVariantMap &obj);
    void remove(int index);
    void setProperty(int index, const QString &property, const QVariant &value);

    // QAbstractItemModel
    QHash<int, QByteArray> roleNames() const;
    bool submit();

private:
    enum Role {
        NameRole = ChatModel::UserRole,
        UsernameRole,
        PasswordRole,
        RealmRole,
        TypeRole
    };

    AccountModelPrivate *d;
    friend class AccountModelPrivate;
};

#endif
