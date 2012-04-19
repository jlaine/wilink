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

#include <QFileInfo>

#include "icons.h"

IconImageProvider::IconImageProvider()
    : QDeclarativeImageProvider(Image)
{
}

QImage IconImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (requestedSize.isValid()) {
        qDebug("req %s %ix%i", qPrintable(id), requestedSize.width(), requestedSize.height());
    } else {
        qDebug("req %s", qPrintable(id));
    }

    QString filePath;
    const QList<int> allSizes = QList<int>() << 128 << 64 << 32 << 16;
    foreach (const int size, allSizes) {
        const QString testPath = QString(":/%1x%2/%3.png").arg(QString::number(size), QString::number(size), id);
        if (QFileInfo(testPath).exists()) {
            filePath = testPath;
        }
    }
    if (filePath.isEmpty())
        filePath = ":/" + id + ".png";

    QImage image(filePath);
    if (requestedSize.isValid())
        image = image.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);

    if (size)
        *size = image.size();
    return image;
}

