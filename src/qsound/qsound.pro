include(qsound.pri)

TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QSOUND_LIBRARY_NAME
VERSION = $$QSOUND_VERSION
DESTDIR = $$QSOUND_LIBRARY_DIR

HEADERS += \
    QSoundFile.h \
    QSoundMeter.h \
    QSoundPlayer.h
SOURCES += \
    QSoundFile.cpp \
    QSoundMeter.cpp \
    QSoundPlayer.cpp
