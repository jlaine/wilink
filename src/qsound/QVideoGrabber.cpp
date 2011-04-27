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

#include <QImage>

#include "QVideoGrabber.h"
#include "QVideoGrabber_p.h"

#define YCBCR_to_RGB(yp, cb, cr) (0xff000000 | \
                                  (quint8(yp + 1.371 * cr) << 16) | \
                                  (quint8(yp - 0.698 * cr - 0.336 * cb) << 8) | \
                                   quint8(yp + 1.732 * cb))

void QVideoGrabber::frameToImage(const QXmppVideoFrame *frame, QImage *image)
{
    if (frame->pixelFormat() == QXmppVideoFrame::Format_RGB32) {
        // copy RGB32 as-is
        memcpy(image->bits(), frame->bits(), frame->mappedBytes());
    } else if (frame->pixelFormat() == QXmppVideoFrame::Format_RGB24) {
        // convert RGB24 to RGB32
        const int width = frame->width();
        const int height = frame->height();
        const int stride = frame->bytesPerLine();
        const quint8 *row = frame->bits();
        QRgb *dest = reinterpret_cast<QRgb*>(image->bits());
        for (int y = 0; y < height; ++y) {
            const uchar *ptr = row;
            for (int x = 0; x < width; ++x) {
                *(dest)++ = 0xff000000 | (ptr[0] << 16) | (ptr[1] << 8) | ptr[2];
                ptr += 3;
            }
            row += stride;
        }
    } else if (frame->pixelFormat() == QXmppVideoFrame::Format_YUV420P) {
        // convert YUV 4:2:0 to RGB32
        const int width = frame->width();
        const int height = frame->height();
        const int stride = frame->bytesPerLine();
        const int c_stride = frame->bytesPerLine() / 2;
        //qDebug("stride %i, cb_stride %i, cr_stride %i", stride, cb_stride, cr_stride);
        const quint8 *y_row = frame->bits();
        const quint8 *cb_row = y_row + (stride * height);
        const quint8 *cr_row = cb_row + (c_stride * height / 2);
        QRgb *dest = reinterpret_cast<QRgb*>(image->bits());
        for (int y = 0; y < height; ++y) {
            const uchar *y_ptr = y_row;
            const uchar *cb_ptr = cb_row;
            const uchar *cr_ptr = cr_row;
            for (int x = 0; x < width; x += 2) {
                const float yp1 = *(y_ptr++);
                const float cb = *(cb_ptr)++ - 128.0;
                const float yp2 = *(y_ptr++);
                const float cr = *(cr_ptr++) - 128.0;
                *(dest++) = YCBCR_to_RGB(yp1, cb, cr);
                *(dest++) = YCBCR_to_RGB(yp2, cb, cr);
            }
            y_row += stride;
            if (y % 2) {
                cb_row += c_stride;
                cr_row += c_stride;
            }
        }
    } else if (frame->pixelFormat() == QXmppVideoFrame::Format_YUYV) {
        // convert YUYV to RGB32
        const int width = frame->width();
        const int height = frame->height();
        const int stride = frame->bytesPerLine();
        const uchar *row = frame->bits();
        QRgb *dest = reinterpret_cast<QRgb*>(image->bits());
        for (int y = 0; y < height; ++y) {
            const uchar *ptr = row;
            for (int x = 0; x < width; x += 2) {
                const float yp1 = *(ptr++);
                const float cb = *(ptr++) - 128.0;
                const float yp2 = *(ptr++);
                const float cr = *(ptr++) - 128.0;
                *(dest++) = YCBCR_to_RGB(yp1, cb, cr);
                *(dest++) = YCBCR_to_RGB(yp2, cb, cr);
            }
            row += stride;
        }
    } else {
        qWarning("Unsupported frame format");
    }
}

QVideoGrabberInfo::QVideoGrabberInfo()
{
    d = new QVideoGrabberInfoPrivate;
}

QVideoGrabberInfo::QVideoGrabberInfo(const QVideoGrabberInfo &other)
{
    d = new QVideoGrabberInfoPrivate;
    *d = *other.d;
}

QVideoGrabberInfo::~QVideoGrabberInfo()
{
    delete d;
}

QVideoGrabberInfo &QVideoGrabberInfo::operator=(const QVideoGrabberInfo &other)
{
    *d = *other.d;
    return *this;
}

QString QVideoGrabberInfo::deviceDescription() const
{
    return d->deviceDescription;
}

QString QVideoGrabberInfo::deviceName() const
{
    return d->deviceName;
}

QList<QXmppVideoFrame::PixelFormat> QVideoGrabberInfo::supportedPixelFormats() const
{
    return d->supportedPixelFormats;
}

