include(idle.pri)

TEMPLATE = lib
CONFIG += staticlib

TARGET = $$IDLE_LIBRARY_NAME

HEADERS = idle.h
SOURCES = idle.cpp
LIBS += $$IDLE_INTERNAL_LIBS

android|symbian|contains(MEEGO_EDITION,harmattan) {
    SOURCES += idle_stub.cpp
} else:mac {
    SOURCES += idle_mac.cpp
} else:unix {
    SOURCES += idle_x11.cpp
    DEFINES += HAVE_XSS
} else:win32 {
    SOURCES += idle_win.cpp
} else {
    SOURCES += idle_stub.cpp
}

