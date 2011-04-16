#include <QByteArray>

#if defined(Q_OS_WIN)
#include <windows.h>
//#include <dshow.h>
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#endif 

class FrameGrabber
{
public:
    FrameGrabber();
    ~FrameGrabber();

    void close();
    bool open();

private:
    int m_fd;
};

FrameGrabber::FrameGrabber()
    : m_fd(-1)
{
}

FrameGrabber::~FrameGrabber()
{
    close();
}

void FrameGrabber::close()
{
#if defined(Q_OS_LINUX)
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
#endif
}


bool FrameGrabber::open()
{
#if defined(Q_OS_LINUX)
    v4l2_capability capability;
    v4l2_format format;

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

    /* get capture image format */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0) {
        qWarning("Could not get format from video device");
        ::close(fd);
        return false;
    }
    qDebug("capture format width: %d, height: %d, pixelformat: %x, %x", format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.pixelformat, V4L2_PIX_FMT_RGB24);
    format.fmt.pix.width = 320;
    format.fmt.pix.height = 240;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    //format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        qWarning("Could not set format from video device");
        ::close(fd);
        return false;
    }
    m_fd = fd;
    return true;
#else
    return false;
#endif
}

int main(int argc, char *argv[])
{
    FrameGrabber grabber;
    if (!grabber.open())
        return -1;
   
    grabber.close();
    return 0;
}

