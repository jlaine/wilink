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

#define RGB_to_Y(r, g, b) ((77 * r1 + 160 * g1 + 29 * b1) / 256)

void QVideoGrabber::convert(const QSize &size,
                            QXmppVideoFrame::PixelFormat inputFormat, const int inputStride, const uchar *input,
                            QXmppVideoFrame::PixelFormat outputFormat, const int outputStride, uchar *output)
{
    const int width = size.width();
    const int height = size.height();
    if (inputFormat == QXmppVideoFrame::Format_RGB24) {
        if (outputFormat == QXmppVideoFrame::Format_RGB32) {
            // convert RGB24 to RGB32
            const uchar *i_row = input;
            QRgb *o_row = reinterpret_cast<QRgb*>(output);
            for (int y = 0; y < height; ++y) {
                const uchar *i_ptr = i_row;
                QRgb *o_ptr = o_row;
                for (int x = 0; x < width; ++x) {
                    *(o_ptr)++ = 0xff000000 | (i_ptr[2] << 16) | (i_ptr[1] << 8) | i_ptr[0];
                    i_ptr += 3;
                }
                i_row += inputStride;
                o_row += outputStride;
            }
        } else if (outputFormat == QXmppVideoFrame::Format_YUYV) {
            // convert RGB24 to YUV 4:2:2
            const uchar *i_row = input;
            uchar *o_row = output;
            for (int y = 0; y < height; ++y) {
                const uchar *i_ptr = i_row;
                uchar *o_ptr = o_row;
                for (int x = 0; x < width; x += 2) {
                    const quint8 b1 = *(i_ptr)++;
                    const quint8 g1 = *(i_ptr)++;
                    const quint8 r1 = *(i_ptr)++;
                    const quint8 b2 = *(i_ptr)++;
                    const quint8 g2 = *(i_ptr)++;
                    const quint8 r2 = *(i_ptr)++;
                    *(o_ptr) = RGB_to_Y(r1, g1, b1);
                    *(o_ptr) = 128 + (- 44 * r1 - 87 * g1 + 131 * b1) / 256;
                    *(o_ptr) = RGB_to_Y(r2, g2, b2);
                    *(o_ptr) = 128 + (131 * r1 - 110 * g2 - 21 * b2) / 256;
                }
                i_row += inputStride;
                o_row += outputStride;
            }
        }
    }
}

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
                *(dest)++ = 0xff000000 | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
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

QString QVideoGrabberInfo::deviceName() const
{
    return d->deviceName;
}

QList<QXmppVideoFrame::PixelFormat> QVideoGrabberInfo::supportedPixelFormats() const
{
    return d->supportedPixelFormats;
}

