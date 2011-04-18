#include <QByteArray>

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

class QVideoGrabber
{
public:
    QVideoGrabber();
    ~QVideoGrabber();

    void close();
    QXmppVideoFrame currentFrame();
    bool isOpen() const;
    bool open();
    QList<QXmppVideoFrame::PixelFormat> supportedFormats() const;

private:
    int m_fd;
    uchar *m_base;
    int m_bytesPerLine;
    int m_frameHeight;
    int m_frameWidth;
    size_t m_size;
    QList<QXmppVideoFrame::PixelFormat> m_supportedFormats;
};

QVideoGrabber::QVideoGrabber()
    : m_fd(-1),
    m_base(0),
    m_bytesPerLine(320),
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
    munmap(m_base, m_size);
    ::close(m_fd);
    m_fd = -1;
#endif
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    QXmppVideoFrame frame;
    if (!isOpen())
        return frame;

#if defined(Q_OS_LINUX)
#endif
    return frame;
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
#if 0
        case V4L2_PIX_FMT_YUYV:
            m_supportedFormats << QXmppVideoFrame::Format_YUYV;
            break;
#endif
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
    //format.fmt.pix.bytesperline = m_bytesPerLine;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("Could not set format from video device");
        ::close(fd);
        return false;
    }
    m_frameWidth = format.fmt.pix.width;
    m_frameHeight = format.fmt.pix.height;
    m_bytesPerLine = format.fmt.pix.bytesperline;

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

    m_fd = fd;
    m_base = (uchar*)base;
    m_size = buffer.length;
    return true;
#else
    return false;
#endif
}

QList<QXmppVideoFrame::PixelFormat> QVideoGrabber::supportedFormats() const
{
    return m_supportedFormats;
}

int main(int argc, char *argv[])
{
    QVideoGrabber grabber;
    if (!grabber.open())
        return -1;

    grabber.currentFrame();   
    grabber.close();
    return 0;
}

