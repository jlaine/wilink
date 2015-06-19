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
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "wallet/dummy.h"

DummyWallet::DummyWallet(QObject *parent) : HashWallet(parent)
{
    qDebug("DummyWallet initialised");
}

bool DummyWallet::read()
{
    const QString filePath = dataPath() + ".dummy";

    /* open file */
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Could not read" << filePath;
        return false;
    }

    /* parse data */
    QDataStream in(&file);
    in >> creds;
    return true;
}

bool DummyWallet::write()
{
    const QString filePath = dataPath() + ".dummy";

    /* open file */
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write" << filePath;
        return false;
    }

    /* write data */
    QDataStream out(&file);
    out << creds;
    return true;
}

class DummyWalletPlugin : public QNetIO::WalletPlugin
{
public:
    QNetIO::Wallet *create(const QString &key) {
        if (key.toLower() == QLatin1String("dummy"))
            return new DummyWallet;
        return NULL;
    };

    QStringList keys() const {
        return QStringList(QLatin1String("dummy"));
    };

    int priority(const QString &key) const {
        Q_UNUSED(key);
        return -1;
    };
};

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_STATIC_PLUGIN2(dummy_wallet, DummyWalletPlugin)
#endif
