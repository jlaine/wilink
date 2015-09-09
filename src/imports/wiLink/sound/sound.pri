INCLUDEPATH += sound

HEADERS += \
    sound/QSoundMeter.h \
    sound/QSoundPlayer.h \
    sound/QSoundStream.h \
    sound/QSoundTester.h \
    sound/QVideoGrabber.h \
    sound/QVideoGrabber_p.h
SOURCES += \
    sound/QSoundMeter.cpp \
    sound/QSoundPlayer.cpp \
    sound/QSoundStream.cpp \
    sound/QSoundTester.cpp \
    sound/QVideoGrabber.cpp

mac {
    OBJECTIVE_SOURCES += sound/QVideoGrabber_mac.mm
    LIBS += -framework QTKit -framework QuartzCore -framework Cocoa
} else:win32 {
    SOURCES += sound/QVideoGrabber_win.cpp
    LIBS += -lole32 -loleaut32 -lwinmm -luuid
} else:unix {
    SOURCES += sound/QVideoGrabber_linux.cpp
} else {
    SOURCES += sound/QVideoGrabber_dummy.cpp
}
