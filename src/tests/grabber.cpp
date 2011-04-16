#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
//#include <dshow.h>
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
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
    m_fd = ::open("/dev/video0", O_RDWR);
    if (m_fd < 0) {
        qWarning("Could not open video device");
        return false;
    }
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

