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

static inline uchar CLAMP(int x)
{
  return ((x > 255) ? 255 : (x < 0) ? 0 : x);
}

#define YCBCR_to_RGB(yp, cb, cr) (0xff000000 | \
                                  (CLAMP(yp + 1.371 * cr) << 16) | \
                                  (CLAMP(yp - 0.698 * cr - 0.336 * cb) << 8) | \
                                   CLAMP(yp + 1.732 * cb))

#define RGB_to_Y(r, g, b) ((77 * r + 150 * g + 29 * b) / 256)
#define RGB_to_CB(r, g, b) ((- 44 * r - 87 * g + 131 * b) / 256)
#define RGB_to_CR(r, g, b) ((131 * r - 110 * g - 21 * b) / 256)

QPair<int,int> QVideoGrabber::byteMetrics(QXmppVideoFrame::PixelFormat format, const QSize &size)
{
    const int width = size.width();
    const int height = size.height();
    int bytesPerLine, mappedBytes;
    if (format == QXmppVideoFrame::Format_RGB32) {
        bytesPerLine = width * 4;
        mappedBytes = bytesPerLine * height;
    } else if (format == QXmppVideoFrame::Format_RGB24) {
        bytesPerLine = width * 3;
        mappedBytes = bytesPerLine * height;
    } else if (format == QXmppVideoFrame::Format_YUYV) {
        bytesPerLine = width * 2;
        mappedBytes = bytesPerLine * height;
    } else if (format == QXmppVideoFrame::Format_YUV420P) {
        bytesPerLine = width * 2;
        mappedBytes = bytesPerLine * height / 2;
    } else {
        bytesPerLine = 0;
        mappedBytes = 0;
    }
    return qMakePair(bytesPerLine, mappedBytes);
}

void QVideoGrabber::convert(const QSize &size,
                            QXmppVideoFrame::PixelFormat inputFormat, const int inputStride, const uchar *input,
                            QXmppVideoFrame::PixelFormat outputFormat, const int outputStride, uchar *output)
{
    const int width = size.width();
    const int height = size.height();

    // convert to RGB32 from any format
    if (outputFormat == QXmppVideoFrame::Format_RGB32) {

        if (inputFormat  == QXmppVideoFrame::Format_RGB32) {

            // copy RGB32
            const int chunk = width * 4;
            const uchar *i_row = input;
            uchar *o_row = output;
            for (int y = 0; y < height; ++y) {
                memcpy(o_row, i_row, chunk);
                i_row += inputStride;
                o_row += outputStride;
            }

        } else if (inputFormat == QXmppVideoFrame::Format_RGB24) {

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
                o_row += outputStride/4;
            }

        } else if (inputFormat == QXmppVideoFrame::Format_YUV420P) {

            // convert YUV 4:2:0 to RGB32
            const int c_stride = inputStride / 2;
            const quint8 *y_row = input;
            const quint8 *cb_row = y_row + (inputStride * height);
            const quint8 *cr_row = cb_row + (c_stride * height / 2);
            QRgb *o_row = reinterpret_cast<QRgb*>(output);
            for (int y = 0; y < height; ++y) {
                const uchar *y_ptr = y_row;
                const uchar *cb_ptr = cb_row;
                const uchar *cr_ptr = cr_row;
                QRgb *o_ptr = o_row;
                for (int x = 0; x < width; x += 2) {
                    const float yp1 = *(y_ptr++);
                    const float cb = *(cb_ptr)++ - 128.0;
                    const float yp2 = *(y_ptr++);
                    const float cr = *(cr_ptr++) - 128.0;
                    *(o_ptr++) = YCBCR_to_RGB(yp1, cb, cr);
                    *(o_ptr++) = YCBCR_to_RGB(yp2, cb, cr);
                }
                y_row += inputStride;
                if (y % 2) {
                    cb_row += c_stride;
                    cr_row += c_stride;
                }
                o_row += outputStride/4;
            }

        } else if (inputFormat == QXmppVideoFrame::Format_YUYV) {

            // convert YUYV to RGB32
            const uchar *i_row = input;
            QRgb *o_row = reinterpret_cast<QRgb*>(output);
            for (int y = 0; y < height; ++y) {
                const uchar *i_ptr = i_row;
                QRgb *o_ptr = o_row;
                for (int x = 0; x < width; x += 2) {
                    const float yp1 = *(i_ptr++);
                    const float cb = *(i_ptr++) - 128.0;
                    const float yp2 = *(i_ptr++);
                    const float cr = *(i_ptr++) - 128.0;
                    *(o_ptr++) = YCBCR_to_RGB(yp1, cb, cr);
                    *(o_ptr++) = YCBCR_to_RGB(yp2, cb, cr);
                }
                i_row += inputStride;
                o_row += outputStride/4;
            }
        }

    // convert to YUYV from selected formats
    } else if (outputFormat == QXmppVideoFrame::Format_YUYV) {
        if (inputFormat == QXmppVideoFrame::Format_RGB24) {
            // convert RGB24 to YUYV
            const uchar *i_row = input;
            uchar *o_row = output;
            for (int y = 0; y < height; ++y) {
                const uchar *i_ptr = i_row;
                uchar *o_ptr = o_row;
                for (int x = 0; x < width; x += 2) {
                    const quint8 b1 = *(i_ptr++);
                    const quint8 g1 = *(i_ptr++);
                    const quint8 r1 = *(i_ptr++);
                    const quint8 b2 = *(i_ptr++);
                    const quint8 g2 = *(i_ptr++);
                    const quint8 r2 = *(i_ptr++);
                    *(o_ptr++) = CLAMP(RGB_to_Y(r1, g1, b1));
                    *(o_ptr++) = CLAMP(128 + RGB_to_CB(r1, g1, b1));
                    *(o_ptr++) = CLAMP(RGB_to_Y(r2, g2, b2));
                    *(o_ptr++) = CLAMP(128 + RGB_to_CR(r1, g1, b1));
                }
                i_row += inputStride;
                o_row += outputStride;
            }
        }
    }
}

void QVideoGrabber::frameToImage(const QXmppVideoFrame *frame, QImage *image)
{
    convert(frame->size(),
            frame->pixelFormat(), frame->bytesPerLine(), frame->bits(),
            QXmppVideoFrame::Format_RGB32, frame->width() * 4, image->bits());
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

