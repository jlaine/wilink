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

#ifndef __QNETIO_WALLET_WINDOWS_H__
#define __QNETIO_WALLET_WINDOWS_H__

#include <QHash>

#include "wallet.h"

/** Support for dummy keyring (in memory only).
 */
class WindowsWallet : public QNetIO::HashWallet
{
    Q_OBJECT

public:
    WindowsWallet(QObject *parent=NULL);
    ~WindowsWallet();

protected:
    bool read();
    bool write();
};

class WindowsWalletPlugin : public QNetIO::WalletPlugin
{
    Q_OBJECT

public:
    QNetIO::Wallet *create(const QString &key);
    QStringList keys() const;
    int priority(const QString &key) const;
};

#endif
