/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include <QByteArray>
#include <QImage>

#include "QVideoGrabber.h"

int main(int argc, char *argv[])
{
    // list available devices
    foreach (const QVideoGrabberInfo &info, QVideoGrabberInfo::availableGrabbers()) {
        qDebug("Device %s", qPrintable(info.deviceName()));
        if (info.supportedPixelFormats().contains(QXmppVideoFrame::Format_YUYV))
            qDebug(" - supports YUYV");
        if (info.supportedPixelFormats().contains(QXmppVideoFrame::Format_YUV420P))
            qDebug(" - supports YUV420P");
    }

    QVideoGrabber grabber;
    if (!grabber.open())
        return -1;

    grabber.start();

    QXmppVideoFrame frame = grabber.currentFrame();

    QImage image(frame.size(), QImage::Format_RGB32);
    image.fill(0);

    // convert YUYV to RGB32
    const int width = frame.width();
    const int height = frame.height();
    const int stride = frame.bytesPerLine();
    const quint8 *row = frame.bits();
    for (int y = 0; y < height; ++y) {
        const quint8 *ptr = row;
        for (int x = 0; x < width; x += 2) {
            const float yp1 = *(ptr++);
            const float cb = *(ptr++) - 128.0;
            const float yp2 = *(ptr++);
            const float cr = *(ptr++) - 128.0;
            image.setPixel(x, y, YCBCR_to_RGB(yp1, cb, cr));
            image.setPixel(x+1, y, YCBCR_to_RGB(yp2, cb, cr));
        }
        row += stride;
    }
    image.save("foo.png");

    grabber.stop();
    grabber.close();
    return 0;
}

