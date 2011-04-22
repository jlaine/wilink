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
#include <QSocketNotifier>

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
    size_t length;
};

class QVideoGrabberPrivate
{
public:
    QVideoGrabberPrivate(QVideoGrabber *qq);
    bool open();
    void close();

    int fd;
    QList<QVideoGrabberBuffer> buffers;
    QXmppVideoFrame currentFrame;
    QString deviceName;
    QSocketNotifier *notifier;
    QXmppVideoFormat videoFormat;

private:
    QVideoGrabber *q;
};

QVideoGrabberPrivate::QVideoGrabberPrivate(QVideoGrabber *qq)
    : fd(-1),
    notifier(0),
    q(qq)
{
}

void QVideoGrabberPrivate::close()
{
    if (fd < 0)
        return;

    // stop watching socket
    if (notifier) {
        delete notifier;
        notifier = 0;
    }

    // close device
    foreach (const QVideoGrabberBuffer &b, buffers)
        munmap(b.base, b.length);
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

    // query capabilities
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

    // get capture image format
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not get format from device", qPrintable(deviceName));
        close();
        return false;
    }
    format.fmt.pix.width = videoFormat.frameWidth();
    format.fmt.pix.height = videoFormat.frameHeight();
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("QVideoGrabber(%s): could not set format to device", qPrintable(deviceName));
        close();
        return false;
    }

    qDebug("QVideoGrabber(%s) capture format width: %d, height: %d, bytesperline: %d, pixelformat: %x", qPrintable(deviceName), format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.bytesperline, format.fmt.pix.pixelformat);

    // request buffers
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 5;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        qWarning("QVideoGrabber(%s): could not request buffers", qPrintable(deviceName));
        close();
        return false;
    }

    // map buffers
    v4l2_buffer handle;
    memset(&handle, 0, sizeof(handle));
    handle.type = reqbuf.type;
    handle.memory = reqbuf.memory;
    for (handle.index = 0; handle.index < reqbuf.count; ++handle.index) {
        QVideoGrabberBuffer buffer;
        if (ioctl(fd, VIDIOC_QUERYBUF, &handle) < 0) {
            qWarning("QVideoGrabber(%s): could not query buffer %i", qPrintable(deviceName), handle.index);
            close();
            return false;
        }
        buffer.base = (uchar*)mmap(NULL, handle.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, handle.m.offset);
        buffer.length = handle.length;
        if (buffer.base == MAP_FAILED) {
            qWarning("QVideoGrabber(%s): could not map buffer %i", qPrintable(deviceName), handle.index);
            close();
            return false;
        }
        buffers << buffer;
    }

    // watch file descriptor
    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, q);

    // store config
    const int frameBytes = buffers.first().length;
    videoFormat.setFrameSize(QSize(format.fmt.pix.width, format.fmt.pix.height));
    videoFormat.setPixelFormat(v4l_to_qxmpp_PixelFormat(format.fmt.pix.pixelformat));
    currentFrame = QXmppVideoFrame(frameBytes, videoFormat.frameSize(), format.fmt.pix.bytesperline, videoFormat.pixelFormat());
    return true;
}

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate(this);
    d->deviceName = "/dev/video0";
    d->videoFormat = format;
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
    return d->currentFrame;
}

QXmppVideoFormat QVideoGrabber::format() const
{
    return d->videoFormat;
}

void QVideoGrabber::onFrameCaptured()
{
    if (d->fd < 0)
        return;

    v4l2_buffer handle;
    memset(&handle, 0, sizeof(handle));
    handle.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    handle.memory = V4L2_MEMORY_MMAP;

    if (ioctl(d->fd, VIDIOC_DQBUF, &handle) < 0) {
        qWarning("QVideoGrabber(%s): could not dequeue buffer", qPrintable(d->deviceName));
        return;
    }

    Q_ASSERT(handle.index < d->buffers.size());
    memcpy(d->currentFrame.bits(), d->buffers[handle.index].base, d->buffers[handle.index].length);

    if (ioctl(d->fd, VIDIOC_QBUF, &handle) < 0)
        qWarning("QVideoGrabber(%s): could not queue buffer %i", qPrintable(d->deviceName), handle.index);

    emit readyRead();
}

bool QVideoGrabber::start()
{
    if (d->fd < 0 && !d->open())
        return false;

    v4l2_buffer handle;
    memset(&handle, 0, sizeof(handle));
    handle.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    handle.memory = V4L2_MEMORY_MMAP;
    for (handle.index = 0; handle.index < d->buffers.size(); ++handle.index) {
        if (ioctl(d->fd, VIDIOC_QBUF, &handle) < 0) {
            qWarning("QVideoGrabber(%s): could not queue buffer %i", qPrintable(d->deviceName), handle.index);
            return false;
        }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(d->fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("QVideoGrabber(%s): could not start streaming", qPrintable(d->deviceName));
        return false;
    }

    connect(d->notifier, SIGNAL(activated(int)),
            this, SLOT(onFrameCaptured()));
    return true;
}

void QVideoGrabber::stop()
{
    disconnect(d->notifier, SIGNAL(activated(int)),
               this, SLOT(onFrameCaptured()));

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

