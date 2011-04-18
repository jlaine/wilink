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

#include "video_grabber.h"
#include <QByteArray>
#include <QImage>

#include "QXmppRtpChannel.h"

#if defined(Q_OS_WIN)
#include <windows.h>
//#include <dshow.h>
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#endif 

#define YCBCR_to_RGB(yp, cb, cr) ((quint8(yp + 1.371 * cr) << 16) | \
                                  (quint8(yp - 0.698 * cr - 0.336 * cb) << 8) | \
                                   quint8(yp + 1.732 * cb))
 
QVideoGrabber::QVideoGrabber()
    : m_fd(-1),
    m_base(0),
    m_frameHeight(240),
    m_frameWidth(320)
{
}

QVideoGrabber::~QVideoGrabber()
{
    close();
}

void QVideoGrabber::close()
{
    if (!isOpen())
        return;
#if defined(Q_OS_LINUX)
    munmap(m_base, m_mappedBytes);
    ::close(m_fd);
    m_fd = -1;
#endif
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    if (!isOpen())
        return QXmppVideoFrame();

#if defined(Q_OS_LINUX)
    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (ioctl(m_fd, VIDIOC_DQBUF, &buffer) < 0) {
        qWarning("Could not dequeue buffer %i", buffer.index);
        return QXmppVideoFrame();
    }

    QXmppVideoFrame frame(m_mappedBytes, QSize(m_frameWidth, m_frameHeight), m_bytesPerLine, m_pixelFormat);
    memcpy(frame.bits(), m_base, m_mappedBytes);
    qDebug("mapped bytes %i", m_mappedBytes);
    return frame;
#else
    return QXmppVideoFrame();
#endif
}

bool QVideoGrabber::isOpen() const
{
#if defined(Q_OS_LINUX)
    return m_fd >= 0;
#endif
    return false;
}

bool QVideoGrabber::open()
{
#if defined(Q_OS_LINUX)
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
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        qDebug("Supported format %s", fmtdesc.description);
        switch (fmtdesc.pixelformat) {
        case V4L2_PIX_FMT_YUYV:
            m_supportedFormats << QXmppVideoFrame::Format_YUYV;
            break;
        case V4L2_PIX_FMT_YUV420:
            m_supportedFormats << QXmppVideoFrame::Format_YUV420P;
            break;
        }
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
    format.fmt.pix.width = m_frameWidth;
    format.fmt.pix.height = m_frameHeight;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("Could not set format from video device");
        ::close(fd);
        return false;
    }
    m_bytesPerLine = format.fmt.pix.bytesperline;
    m_frameWidth = format.fmt.pix.width;
    m_frameHeight = format.fmt.pix.height;
    m_pixelFormat = QXmppVideoFrame::Format_YUYV;

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

    /* start */
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("Could not start streaming");
        ::close(fd);
        return false;
    }

    m_fd = fd;
    m_base = (uchar*)base;
    m_mappedBytes = buffer.length;
    return true;
#else
    return false;
#endif
}

bool QVideoGrabber::start()
{
#if defined(Q_OS_LINUX)
    const v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (ioctl(m_fd, VIDIOC_QBUF, &buffer) < 0) {
        qWarning("Could not queue buffer %i", buffer.index);
        return false;
    }

    if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning("Could not start streaming");
        return false;
    }
    return true;
#else
    return false;
#endif
}

void QVideoGrabber::stop()
{
#if defined(Q_OS_LINUX)
    const v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) < 0)
        qWarning("Could not stop streaming");
#endif
}

QList<QXmppVideoFrame::PixelFormat> QVideoGrabber::supportedFormats() const
{
    return m_supportedFormats;
}

