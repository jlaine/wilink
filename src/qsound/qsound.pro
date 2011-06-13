include(qsound.pri)
include(../qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QSOUND_LIBRARY_NAME
VERSION = $$QSOUND_VERSION
DESTDIR = $$QSOUND_LIBRARY_DIR

INCLUDEPATH += $$QXMPP_INCLUDE_DIR

android {
    HEADERS += fake/QAudioOutput
}

HEADERS += \
    QSoundFile.h \
    QSoundMeter.h \
    QSoundPlayer.h \
    QVideoGrabber.h \
    QVideoGrabber_p.h
SOURCES += \
    QSoundFile.cpp \
    QSoundMeter.cpp \
    QSoundPlayer.cpp \
    QVideoGrabber.cpp \
    QVideoGrabber_dummy.cpp
