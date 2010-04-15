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
#include <QTime>

#define HASH_BLOCK_SIZE 16384

int main(int argc, char *argv[])
{
    char buffer[HASH_BLOCK_SIZE];
    QCryptographicHash hasher(QCryptographicHash::Md5);
    QTime t;

    for (int i = 1; i < argc; i++)
    {
        const QString filePath(argv[i]);
        int len;

        t.start();
        QFile fp(filePath);
        fp.open(QIODevice::ReadOnly);
        while ((len = fp.read(buffer, HASH_BLOCK_SIZE)) > 0)
            hasher.addData(buffer, len);
        fp.close();
        qDebug() << "hashed" << filePath << hasher.result().toHex() << "in" << t.elapsed();
    }
    return 0;
}

