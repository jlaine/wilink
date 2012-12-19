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

#ifndef __QNETIO_WALLET_DUMMY_H__
#define __QNETIO_WALLET_DUMMY_H__

#include <QHash>

#include "wallet.h"

/** Support for dummy keyring (in memory only).
 */
class DummyWallet : public QNetIO::HashWallet
{
    Q_OBJECT

public:
    DummyWallet(QObject *parent=NULL);

protected:
    bool read();
    bool write();
};

#endif
