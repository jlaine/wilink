android {
    SOURCES += idle/idle_stub.cpp
} else:mac {
    LIBS += -framework Carbon
    SOURCES += idle/idle_mac.cpp
} else:unix {
    QT += dbus x11extras
    DEFINES += HAVE_XSS
    SOURCES += idle/idle_x11.cpp
    LIBS += -lXss -lX11
} else:win32 {
    SOURCES += idle/idle_win.cpp
} else {
    SOURCES += idle/idle_stub.cpp
}
