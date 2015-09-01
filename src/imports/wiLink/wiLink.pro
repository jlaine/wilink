include(../../../wilink.pri)
include(../../qnetio/qnetio.pri)
include(../../qsound/qsound.pri)
include(../../3rdparty/qdjango/qdjango.pri)
include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin

QT += declarative network sql xml

TARGET = qmlwilinkplugin

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
    SOURCES += idle/idle_stub.cpp
} else:mac {
    LIBS += -framework Carbon
    SOURCES += idle/idle_mac.cpp
} else:unix {
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += x11extras
    }
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
    menubar.cpp \
    model.cpp \
    notifications.cpp \
    phone.cpp \
    phone/sip.cpp \
    places.cpp \
    rooms.cpp \
    roster.cpp \
    settings.cpp \
    systeminfo.cpp \
    translations.cpp \
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
    menubar.h \
    model.h \
    notifications.h \
    phone.h \
    places.h \
    phone/sip.h \
    phone/sip_p.h \
    rooms.h \
    roster.h \
    settings.h \
    systeminfo.h \
    translations.h \
    updater.h

RESOURCES += ../../data/wiLink.qrc

INCLUDEPATH += \
    $$WILINK_INCLUDE_DIR \
    $$QNETIO_INCLUDE_DIR \
    $$QSOUND_INCLUDE_DIR \
    $$QXMPP_INCLUDEPATH \
    ../../3rdparty/qdjango/src/db \
    ../../qxmpp-extra/diagnostics

LIBS += \
    -L../../qnetio/src $$QNETIO_LIBS \
    -L../../qsound/src -L../../3rdparty/libmad $$QSOUND_LIBS \
    -L../../3rdparty/qxmpp/src $$QXMPP_LIBS \
    -L../../qxmpp-extra -lqxmpp-extra \
    -L../../3rdparty/qdjango/src/db $$QDJANGO_DB_LIBS

android {
    qmldir.path = /assets/imports/wiLink
    qmldir.files = qmldir
    INSTALLS += qmldir
} else:unix {
    qmldir.path = $$PREFIX/lib/qt4/imports/wiLink
    qmldir.files = qmldir
    target.path = $$PREFIX/lib/qt4/imports/wiLink
    INSTALLS += qmldir target
}
