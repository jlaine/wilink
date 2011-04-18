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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include <QByteArray>
#include <QDir>

#include "QVideoGrabber.h"
#include "QVideoGrabber_p.h"

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
    QString deviceName;
    int frameHeight;
    int frameWidth;
    size_t mappedBytes;
    QXmppVideoFrame::PixelFormat pixelFormat;
};

QVideoGrabber::QVideoGrabber()
{
    d = new QVideoGrabberPrivate;
    d->base = 0;
    d->deviceName = "/dev/video0";
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

    int fd = ::open(d->deviceName.toAscii().constData(), O_RDWR);
    if (fd < 0) {
        qWarning("QVideoGrabber(%s): could not open device", qPrintable(d->deviceName));
        return false;
    }

    /* query capabilities */
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        qWarning("QVideoGrabber(%s): could not query capabilities", qPrintable(d->deviceName));
        ::close(fd);
        return false;
    }
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qWarning("QVideoGrabber(%s): video device is not a capture device", qPrintable(d->deviceName));
        ::close(fd);
        return false;
    }
    if (capability.capabilities & V4L2_CAP_READWRITE)
        qDebug("Video device supports read/write");
    if (capability.capabilities & V4L2_CAP_STREAMING)
        qDebug("Video device supports streaming I/O");

    /* get capture image format */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not get format from device", qPrintable(d->deviceName));
        ::close(fd);
        return false;
    }
    qDebug("capture format width: %d, height: %d, bytesperline: %d, pixelformat: %x", format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.bytesperline);
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not set format to device", qPrintable(d->deviceName));
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
        qWarning("QVideoGrabber(%s): could not request buffers", qPrintable(d->deviceName));
        ::close(fd);
        return false;
    }

    /* map buffer */
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = reqbuf.type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0) {
        qWarning("QVideoGrabber(%s): could not query buffer %i", qPrintable(d->deviceName), buffer.index);
        ::close(fd);
        return false;
    }
    void *base = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
    if (base == MAP_FAILED) {
        qWarning("QVideoGrabber(%s): could not map buffer %i", qPrintable(d->deviceName), buffer.index);
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
        qWarning("QVideoGrabber(%s): could not queue buffer %i", qPrintable(d->deviceName), buffer.index);
        return false;
    }

    if (ioctl(d->fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("QVideoGrabber(%s): could not start streaming", qPrintable(d->deviceName));
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

QList<QVideoGrabberInfo> QVideoGrabberInfo::availableGrabbers()
{
    QList<QVideoGrabberInfo> grabbers;
    v4l2_capability capability;
    v4l2_fmtdesc fmtdesc;


    QDir dev("/dev");
    dev.setNameFilters(QStringList() << "video*");
    dev.setFilter(QDir::Files | QDir::System);
    foreach (const QString &device, dev.entryList()) {
        QVideoGrabberInfo grabber;
        grabber.d->deviceName = dev.filePath(device);

        // query capabilities
        int fd = ::open(grabber.d->deviceName.toAscii().constData(), O_RDWR);
        if (fd < 0)
            continue;
        if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
            ::close(fd);
            continue;
        }
        if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            ::close(fd);
            continue;
        }
        //qDebug("Card: %s, driver: %s", capability.card, capability.driver);

        // query pixel formats
        QList<QXmppVideoFrame::PixelFormat> pixelFormats;
        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
            //qDebug("Supported format %s", fmtdesc.description);
            QXmppVideoFrame::PixelFormat format = v4l_to_qxmpp_PixelFormat(fmtdesc.pixelformat);
            if (format != QXmppVideoFrame::Format_Invalid &&
                !grabber.d->supportedPixelFormats.contains(format))
                grabber.d->supportedPixelFormats << format;
            fmtdesc.index++;
        }

        ::close(fd);
        grabbers << grabber;
    }

    return grabbers;
}

