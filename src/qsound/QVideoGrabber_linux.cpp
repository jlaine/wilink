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

#include "QVideoGrabber.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

static QXmppVideoFrame::PixelFormat v4l_to_qxmpp_PixelFormat(__u32 pixelFormat)
{
    switch (pixelFormat) {
    case V4L2_PIX_FMT_YUYV:
        return QXmppVideoFrame::Format_YUYV;
    case V4L2_PIX_FMT_YUV420:
        return QXmppVideoFrame::Format_YUV420P;
    default:
        return QXmppVideoFrame::Format_Invalid;
    }
}

class QVideoGrabberPrivate
{
public:
    int fd;
    uchar *base;
    int bytesPerLine;
    int frameHeight;
    int frameWidth;
    size_t mappedBytes;
    QXmppVideoFrame::PixelFormat pixelFormat;
    QList<QXmppVideoFrame::PixelFormat> supportedFormats;
};

QVideoGrabber::QVideoGrabber()
{
    d = new QVideoGrabberPrivate;
    d->base = 0;
    d->fd = -1;
    d->frameWidth = 0;
    d->frameHeight = 0;
}

QVideoGrabber::~QVideoGrabber()
{
    close();
    delete d;
}

void QVideoGrabber::close()
{
    if (d->fd < 0)
        return;

    // stop acquisition
    stop();

    // close device
    munmap(d->base, d->mappedBytes);
    ::close(d->fd);
    d->fd = -1;
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    if (d->fd < 0)
        return QXmppVideoFrame();

    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (ioctl(d->fd, VIDIOC_DQBUF, &buffer) < 0) {
        qWarning("Could not dequeue buffer %i", buffer.index);
        return QXmppVideoFrame();
    }

    QXmppVideoFrame frame(d->mappedBytes, QSize(d->frameWidth, d->frameHeight), d->bytesPerLine, d->pixelFormat);
    memcpy(frame.bits(), d->base, d->mappedBytes);
    return frame;
}

bool QVideoGrabber::isOpen() const
{
    return d->fd >= 0;
}

bool QVideoGrabber::open()
{
    v4l2_buffer buffer;
    v4l2_capability capability;
    v4l2_format format;
    v4l2_requestbuffers reqbuf;

    int fd = ::open("/dev/video0", O_RDWR);
    if (fd < 0) {
        qWarning("Could not open video device");
        return false;
    }

    /* query capabilities */
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        qWarning("Could not query video device capabilities");
        ::close(fd);
        return false;
    }
    qDebug("card: %s, driver: %s", capability.card, capability.driver);
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qWarning("Video device is not a capture device");
        ::close(fd);
        return false;
    }
    if (capability.capabilities & V4L2_CAP_READWRITE)
        qDebug("Video device supports read/write");
    if (capability.capabilities & V4L2_CAP_STREAMING)
        qDebug("Video device supports streaming I/O");

    /* get supported formats */
    QList<QXmppVideoFrame::PixelFormat> pixelFormats;
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        qDebug("Supported format %s", fmtdesc.description);
        QXmppVideoFrame::PixelFormat format = v4l_to_qxmpp_PixelFormat(fmtdesc.pixelformat);
        if (format != QXmppVideoFrame::Format_Invalid && !pixelFormats.contains(format))
            pixelFormats << format;
        fmtdesc.index++;
    }

    /* get capture image format */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
        qWarning("Could not get format from video device");
        ::close(fd);
        return false;
    }
    qDebug("capture format width: %d, height: %d, bytesperline: %d, pixelformat: %x", format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.bytesperline);
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("Could not set format from video device");
        ::close(fd);
        return false;
    }

    qDebug("capture format width: %d, height: %d, bytesperline: %d, pixelformat: %x", format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.bytesperline, format.fmt.pix.pixelformat);

    /* request buffers */
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 1;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        qWarning("Could not request buffers");
        ::close(fd);
        return false;
    }

    /* map buffer */
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = reqbuf.type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0) {
        qWarning("Could not query buffer");
        ::close(fd);
        return false;
    }
    void *base = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
    if (base == MAP_FAILED) {
        qWarning("Could not map buffer");
        ::close(fd);
        return false;
    }

    /* store config */
    d->bytesPerLine = format.fmt.pix.bytesperline;
    d->fd = fd;
    d->frameWidth = format.fmt.pix.width;
    d->frameHeight = format.fmt.pix.height;
    d->base = (uchar*)base;
    d->mappedBytes = buffer.length;
    d->pixelFormat = v4l_to_qxmpp_PixelFormat(format.fmt.pix.pixelformat);
    d->supportedFormats  = pixelFormats;
    return true;
}

bool QVideoGrabber::start()
{
    const v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (ioctl(d->fd, VIDIOC_QBUF, &buffer) < 0) {
        qWarning("Could not queue buffer %i", buffer.index);
        return false;
    }

    if (ioctl(d->fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("Could not start streaming");
        return false;
    }
    return true;
}

void QVideoGrabber::stop()
{
    const v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(d->fd, VIDIOC_STREAMOFF, &type) < 0)
        qWarning("Could not stop streaming");
}

QList<QXmppVideoFrame::PixelFormat> QVideoGrabber::supportedFormats() const
{
    return d->supportedFormats;
}

