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
HEADERS += wallet.h
SOURCES += wallet.cpp

mac {
    DEFINES += USE_OSX_KEYCHAIN
    HEADERS += wallet/osx.h
    SOURCES += wallet/osx.cpp
    LIBS += -framework Foundation -framework Security
} else:win32 {
    DEFINES += USE_WINDOWS_KEYRING
    HEADERS += wallet/windows.h
    SOURCES += wallet/windows.cpp
} else {
    HEADERS += wallet/dummy.h
    SOURCES += wallet/dummy.cpp
}
