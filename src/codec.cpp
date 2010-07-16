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

#include <cstdlib>

#include <QDebug>
#include <QFile>

#include "qxmpp/QXmppCodec.h"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        qWarning() << "Missing arguments";
        return EXIT_FAILURE;
    }

    // open input
    QFile input(QString::fromLocal8Bit(argv[1]));
    if (!input.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open input file" << input.fileName();
        return EXIT_FAILURE;
    }
    QDataStream inputStream(&input);
    inputStream.setByteOrder(QDataStream::LittleEndian);

    // open output
    QFile output(QString::fromLocal8Bit(argv[2]));
    if (!output.open(QIODevice::WriteOnly))
    {
        qDebug() << "Could not open output file" << output.fileName();
        return EXIT_FAILURE;
    }
    QDataStream outputStream(&output);
    outputStream.setByteOrder(QDataStream::LittleEndian);

    // perform encoding
    QXmppG711aCodec codec(8000);
    qint64 samples = codec.decode(inputStream, outputStream);
    qDebug() << "transcoded" << samples << "samples";

    return EXIT_SUCCESS;
}

