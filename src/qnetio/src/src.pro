include(../qnetio.pri)

TEMPLATE = lib
CONFIG += staticlib

# Target definition
TARGET = $$QNETIO_LIBRARY_NAME
VERSION = $$QNETIO_VERSION
win32 {
    DESTDIR = $$OUT_PWD
}

INCLUDEPATH += $$QNETIO_INCLUDE_DIR

# Plugins
DEFINES += QT_STATICPLUGIN
HEADERS += \
    mimetypes.h \
    wallet.h \
    wallet/dummy.h
SOURCES += \
    mimetypes.cpp \
    wallet.cpp \
    wallet/dummy.cpp

mac {
    DEFINES += USE_OSX_KEYCHAIN
    HEADERS += wallet/osx.h
    SOURCES += wallet/osx.cpp
    LIBS += -framework Foundation -framework Security
} else:win32 {
    DEFINES += USE_WINDOWS_KEYRING
    HEADERS += wallet/windows.h
    SOURCES += wallet/windows.cpp
}
