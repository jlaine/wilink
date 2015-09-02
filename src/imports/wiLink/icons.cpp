/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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
    : QQuickImageProvider(Image)
{
}

QImage IconImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString filePath;
    const QList<int> allSizes = QList<int>() << 128 << 64 << 32 << 16;
    foreach (const int size, allSizes) {
        const QString testPath = QString(":/images/%1x%2/%3.png").arg(QString::number(size), QString::number(size), id);
        const bool sizeSufficient = !requestedSize.isValid() || (size >= requestedSize.width() && size >= requestedSize.height());
        if ((filePath.isEmpty() || sizeSufficient) && QFileInfo(testPath).exists()) {
            filePath = testPath;
        }
    }
    if (filePath.isEmpty())
        filePath = ":/images/" + id + ".png";

    QImage image(filePath);
    if (requestedSize.isValid() && requestedSize != image.size())
        image = image.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (size)
        *size = image.size();
    return image;
}

