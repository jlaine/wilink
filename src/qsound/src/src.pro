include(../qsound.pri)
include(../../qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QSOUND_LIBRARY_NAME
VERSION = $$QSOUND_VERSION
DESTDIR = $$QSOUND_LIBRARY_DIR

DEFINES += $$QSOUND_INTERNAL_DEFINES
INCLUDEPATH += $$QSOUND_INCLUDE_DIR $$QXMPP_INCLUDE_DIR
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
    QSoundMeter.h \
    QSoundPlayer.h \
    QSoundStream.h \
    QSoundTester.h \
    QVideoGrabber.h \
    QVideoGrabber_p.h
SOURCES += \
    QSoundFile.cpp \
    QSoundMeter.cpp \
    QSoundPlayer.cpp \
    QSoundStream.cpp \
    QSoundTester.cpp \
    QVideoGrabber.cpp \
    QVideoGrabber_dummy.cpp
