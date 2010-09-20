TEMPLATE = lib
CONFIG += staticlib

HEADERS = idle.h
SOURCES = idle.cpp

symbian {

} else:mac {
    SOURCES += idle_mac.cpp
    LIBS += "-framework Carbon"
} else:unix {
    SOURCES += idle_x11.cpp
    LIBS += -lXss
    # add_definitions(-DHAVE_XSS)
} else windows {
    SOURCES += idle_win.cpp
}
