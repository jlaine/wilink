include(qsound.pri)

TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QSOUND_LIBRARY_NAME
VERSION = $$QSOUND_VERSION
DESTDIR = $$QSOUND_LIBRARY_DIR

HEADERS += \
    QSoundFile.h \
    QSoundPlayer.h
SOURCES += \
    QSoundFile.cpp \
    QSoundPlayer.cpp
