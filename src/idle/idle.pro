include(idle.pri)

TEMPLATE = lib
CONFIG += staticlib

HEADERS = idle.h
SOURCES = idle.cpp
LIBS += $$IDLE_INTERNAL_LIBS

symbian {

} else:mac {
    SOURCES += idle_mac.cpp
} else:unix {
    SOURCES += idle_x11.cpp
    # add_definitions(-DHAVE_XSS)
} else:win32 {
    SOURCES += idle_win.cpp
}
