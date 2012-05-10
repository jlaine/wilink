include(../../../wilink.pri)
include(../../qnetio/qnetio.pri)
include(../../qsound/qsound.pri)
include(../../3rdparty/qdjango/qdjango.pri)
include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin

QT += declarative network sql xml

TARGET = wiLink

# FIXME: this is a hack so that Q_OS_ANDROID is defined
android {
    DEFINES += ANDROID
} else:unix {
    QT += dbus
}

# embedded version
android|symbian|contains(MEEGO_EDITION,harmattan) {
    DEFINES += WILINK_EMBEDDED
}

# Libraries used internal by idle
android|symbian|contains(MEEGO_EDITION,harmattan) {
    SOURCES += idle_stub.cpp
} else:mac {
    LIBS += -framework Carbon
    SOURCES += idle/idle_mac.cpp
} else:unix {
    DEFINES += HAVE_XSS
    SOURCES += idle/idle_x11.cpp
    LIBS += -lXss -lX11
} else:win32 {
    SOURCES += idle/idle_win.cpp
} else {
    SOURCES += idle/idle_stub.cpp
}

SOURCES += \
    accounts.cpp \
    calls.cpp \
    client.cpp \
    console.cpp \
    conversations.cpp \
    declarative.cpp \
    diagnostics.cpp \
    discovery.cpp \
    history.cpp \
    icons.cpp \
    idle/idle.cpp \
    model.cpp \
    news.cpp \
    notifications.cpp \
    phone.cpp \
    phone/sip.cpp \
    photos.cpp \
    places.cpp \
    player.cpp \
    rooms.cpp \
    roster.cpp \
    settings.cpp \
    shares.cpp \
    systeminfo.cpp \
    updater.cpp

HEADERS += \
    accounts.h \
    calls.h \
    client.h \
    console.h \
    conversations.h \
    declarative.h \
    declarative_qxmpp.h \
    diagnostics.h \
    discovery.h \
    history.h \
    icons.h \
    idle/idle.h \
    model.h \
    news.h \
    notifications.h \
    photos.h \
    phone.h \
    places.h \
    player.h \
    phone/sip.h \
    phone/sip_p.h \
    rooms.h \
    roster.h \
    settings.h \
    shares.h \
    systeminfo.h \
    updater.h

INCLUDEPATH += \
    $$WILINK_INCLUDE_DIR \
    $$QNETIO_INCLUDE_DIR \
    $$QSOUND_INCLUDE_DIR \
    $$QXMPP_INCLUDEPATH \
    ../../3rdparty/qdjango/src/db \
    ../../qxmpp-extra/diagnostics \
    ../../qxmpp-extra/shares

LIBS += \
    -L../../qnetio/src $$QNETIO_LIBS \
    -L../../qsound/src $$QSOUND_LIBS \
    -L../../3rdparty/qxmpp/src $$QXMPP_LIBS \
    -L../../qxmpp-extra -lqxmpp-extra \
    -L../../3rdparty/qdjango/src/db $$QDJANGO_DB_LIBS

