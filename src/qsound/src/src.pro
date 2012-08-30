include(../qsound.pri)
include(../../3rdparty/qxmpp/qxmpp.pri)

QT += declarative
TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QSOUND_LIBRARY_NAME
VERSION = $$QSOUND_VERSION
win32 {
    DESTDIR = $$OUT_PWD
}

DEFINES += $$QSOUND_INTERNAL_DEFINES
INCLUDEPATH += $$QSOUND_INCLUDE_DIR $$QXMPP_INCLUDEPATH
LIBS += $$QSOUND_INTERNAL_LIBS

android {
    HEADERS += \
        fake/QAudioDeviceInfo \
        fake/QAudioFormat \
        fake/QAudioInput \
        fake/QAudioOutput
}

HEADERS += \
    QSoundFile.h \
    QSoundLoader.h \
    QSoundMeter.h \
    QSoundPlayer.h \
    QSoundStream.h \
    QSoundTester.h \
    QVideoGrabber.h \
    QVideoGrabber_p.h
SOURCES += \
    QSoundFile.cpp \
    QSoundLoader.cpp \
    QSoundMeter.cpp \
    QSoundPlayer.cpp \
    QSoundStream.cpp \
    QSoundTester.cpp \
    QVideoGrabber.cpp

mac {
    SOURCES += QVideoGrabber_mac.mm
} else:win32 {
    SOURCES += QVideoGrabber_win.cpp
} else {
    SOURCES += QVideoGrabber_dummy.cpp
}
