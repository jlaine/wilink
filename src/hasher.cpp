/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTime>

/** Fingerprint a file.
 */
static QByteArray hashFile(const QString &path)
{
    QFile fp(path);

    if (!fp.open(QIODevice::ReadOnly) || fp.isSequential())
        return QByteArray();

    QCryptographicHash hasher(QCryptographicHash::Md5);
    char buffer[16384];
    int hashed = 0;
    while (fp.bytesAvailable())
    {
        int len = fp.read(buffer, sizeof(buffer));
        if (len < 0)
            return QByteArray();
        hasher.addData(buffer, len);
        hashed += len;
    }
    return hasher.result();
}

QByteArray hmac(QCryptographicHash::Algorithm algorithm, const QByteArray &key, const QByteArray &text)
{
    QCryptographicHash hasher(algorithm);

    const int B = 64;
    QByteArray kpad = key + QByteArray(B - key.size(), 0);

    QByteArray ba;
    for (int i = 0; i < B; ++i)
        ba += kpad[i] ^ 0x5c;

    QByteArray tmp;
    for (int i = 0; i < B; ++i)
        tmp += kpad[i] ^ 0x36;
    hasher.addData(tmp);
    hasher.addData(text);
    ba += hasher.result();

    hasher.reset();
    hasher.addData(ba);
    return hasher.result();
}

int main(int argc, char *argv[])
{
    QByteArray h = hmac(QCryptographicHash::Md5, QByteArray(16, 0x0b), QByteArray("Hi There"));
    Q_ASSERT(h.toHex() == "9294727a3638bb1c13f48ef8158bfc9d");

    h = hmac(QCryptographicHash::Md5, QByteArray("Jefe"), QByteArray("what do ya want for nothing?"));
    Q_ASSERT(h.toHex() == "750c783e6ab0b503eaa86e310a5db738");

    h = hmac(QCryptographicHash::Md5, QByteArray(16, 0xaa), QByteArray(50, 0xdd));
    Q_ASSERT(h.toHex() == "56be34521d144c88dbb8c733f0e8b3f6");

    for (int i = 1; i < argc; ++i)
    {
        QString path = QString::fromLocal8Bit(argv[i]);
        QFileInfo info(path);
        QTime t;
        t.start();
        QByteArray result = hashFile(path);
        int ms = t.elapsed();
        if (ms > 0)
            qDebug() << info.fileName() << result.toHex() << ms/1000.0 << "s" << (double(info.size()) / (1000.0 * ms)) << "MB/s";
    }
    return 0;
}

