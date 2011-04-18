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

struct QVideoGrabberBuffer
{
    uchar *base;
    v4l2_buffer handle;
};

class QVideoGrabberPrivate
{
public:
    QVideoGrabberPrivate(QVideoGrabber *qq);
    bool open();
    void close();

    int fd;
    QList<QVideoGrabberBuffer> buffers;
    int bufferTail;
    int bytesPerLine;
    QString deviceName;
    int frameHeight;
    int frameWidth;
    QXmppVideoFrame::PixelFormat pixelFormat;

private:
    QVideoGrabber *q;
};

QVideoGrabberPrivate::QVideoGrabberPrivate(QVideoGrabber *qq)
    : fd(-1),
    bufferTail(0),
    frameWidth(0),
    frameHeight(0),
    q(qq)
{
}

void QVideoGrabberPrivate::close()
{
    if (fd < 0)
        return;

    // close device
    foreach (const QVideoGrabberBuffer &b, buffers)
        munmap(b.base, b.handle.length);
    buffers.clear();
    ::close(fd);
    fd = -1;
}

bool QVideoGrabberPrivate::open()
{
    v4l2_buffer buffer;
    v4l2_capability capability;
    v4l2_format format;
    v4l2_requestbuffers reqbuf;

    fd = ::open(deviceName.toAscii().constData(), O_RDWR);
    if (fd < 0) {
        qWarning("QVideoGrabber(%s): could not open device", qPrintable(deviceName));
        return false;
    }

    /* query capabilities */
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        qWarning("QVideoGrabber(%s): could not query capabilities", qPrintable(deviceName));
        close();
        return false;
    }
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qWarning("QVideoGrabber(%s): video device is not a capture device", qPrintable(deviceName));
        close();
        return false;
    }
    if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
        qDebug("Video device supports streaming I/O");
        close();
        return false;
    }

    /* get capture image format */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not get format from device", qPrintable(deviceName));
        close();
        return false;
    }
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not set format to device", qPrintable(deviceName));
        close();
        return false;
    }

    qDebug("QVideoGrabber(%s) capture format width: %d, height: %d, bytesperline: %d, pixelformat: %x", qPrintable(deviceName), format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.bytesperline, format.fmt.pix.pixelformat);

    /* request buffers */
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 5;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        qWarning("QVideoGrabber(%s): could not request buffers", qPrintable(deviceName));
        close();
        return false;
    }

    /* map buffers */
    for (int i = 0; i < reqbuf.count; ++i) {
        QVideoGrabberBuffer buffer;
        memset(&buffer.handle, 0, sizeof(buffer.handle));
        buffer.handle.type = reqbuf.type;
        buffer.handle.memory = V4L2_MEMORY_MMAP;
        buffer.handle.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buffer.handle) < 0) {
            qWarning("QVideoGrabber(%s): could not query buffer %i", qPrintable(deviceName), buffer.handle.index);
            close();
            return false;
        }
        buffer.base = (uchar*)mmap(NULL, buffer.handle.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.handle.m.offset);
        if (buffer.base == MAP_FAILED) {
            qWarning("QVideoGrabber(%s): could not map buffer %i", qPrintable(deviceName), buffer.handle.index);
            close();
            return false;
        }
        buffers << buffer;
    }

    /* store config */
    bytesPerLine = format.fmt.pix.bytesperline;
    frameWidth = format.fmt.pix.width;
    frameHeight = format.fmt.pix.height;
    pixelFormat = v4l_to_qxmpp_PixelFormat(format.fmt.pix.pixelformat);
    return true;
}


QVideoGrabber::QVideoGrabber()
{
    d = new QVideoGrabberPrivate(this);
    d->deviceName = "/dev/video0";
}

QVideoGrabber::~QVideoGrabber()
{
    // stop acquisition
    stop();

    // close device
    d->close();
    delete d;
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    if (d->fd < 0)
        return QXmppVideoFrame();

    QVideoGrabberBuffer &buffer = d->buffers[d->bufferTail];
    if (ioctl(d->fd, VIDIOC_DQBUF, &buffer.handle) < 0) {
        qWarning("QVideoGrabber(%s): could not dequeue buffer %i", qPrintable(d->deviceName), buffer.handle.index);
        return QXmppVideoFrame();
    }

    QXmppVideoFrame frame(buffer.handle.length, QSize(d->frameWidth, d->frameHeight), d->bytesPerLine, d->pixelFormat);
    memcpy(frame.bits(), buffer.base, buffer.handle.length);

    if (ioctl(d->fd, VIDIOC_QBUF, &buffer.handle) < 0)
        qWarning("QVideoGrabber(%s): could not queue buffer %i", qPrintable(d->deviceName), buffer.handle.index);

    d->bufferTail = (d->bufferTail+1) % d->buffers.size();
    return frame;
}

QXmppVideoFormat QVideoGrabber::format() const
{
    QXmppVideoFormat fmt;
    fmt.setFrameSize(QSize(d->frameWidth, d->frameHeight));
    fmt.setPixelFormat(d->pixelFormat);
    return fmt;
}

bool QVideoGrabber::start()
{
    if (d->fd < 0 && !d->open())
        return false;

    for (int i = 0; i < d->buffers.size(); ++i) {
        QVideoGrabberBuffer &buffer = d->buffers.first();
        if (ioctl(d->fd, VIDIOC_QBUF, &d->buffers[i].handle) < 0) {
            qWarning("QVideoGrabber(%s): could not queue buffer %i", qPrintable(d->deviceName), i);
            return false;
        }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(d->fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("QVideoGrabber(%s): could not start streaming", qPrintable(d->deviceName));
        return false;
    }
    return true;
}

QVideoGrabber::State QVideoGrabber::state() const
{
    return StoppedState;
}

void QVideoGrabber::stop()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(d->fd, VIDIOC_STREAMOFF, &type) < 0)
        qWarning("QVideoGrabber(%s): could not stop streaming", qPrintable(d->deviceName));
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

