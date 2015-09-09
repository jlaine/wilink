INCLUDEPATH += sound

HEADERS += \
    sound/QSoundFile.h \
    sound/QSoundMeter.h \
    sound/QSoundPlayer.h \
    sound/QSoundStream.h \
    sound/QSoundTester.h \
    sound/QVideoGrabber.h \
    sound/QVideoGrabber_p.h
SOURCES += \
    sound/QSoundFile.cpp \
    sound/QSoundMeter.cpp \
    sound/QSoundPlayer.cpp \
    sound/QSoundStream.cpp \
    sound/QSoundTester.cpp \
    sound/QVideoGrabber.cpp

# Libraries used internally by QSound
QSOUND_INTERNAL_DEFINES += QSOUND_USE_MAD
isEmpty(WILINK_SYSTEM_MAD) {
    QSOUND_INCLUDE_DIR += $$PWD/../3rdparty/libmad
}
QSOUND_INTERNAL_LIBS += -lmad

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
