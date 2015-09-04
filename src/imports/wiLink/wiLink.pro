include(../../../wilink.pri)
include(../../qnetio/qnetio.pri)
include(../../qsound/qsound.pri)
include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin

QT += network quick widgets xml

DESTDIR = ../../app/wiLink.2
TARGET = qmlwilinkplugin

# Libraries used internal by idle
android {
    DEFINES += ANDROID
    DEFINES += WILINK_EMBEDDED
    SOURCES += idle/idle_stub.cpp
} else:mac {
    APPDIR = ../../app/wiLink.app
    DESTDIR = $$APPDIR/Contents/Resources/qml/wiLink.2
    LIBS += -framework Carbon
    SOURCES += idle/idle_mac.cpp
    OBJECTIVE_SOURCES += settings_mac.mm
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
    ../../qxmpp-extra/diagnostics

LIBS += \
    -L../../qnetio/src $$QNETIO_LIBS \
    -L../../qsound/src -L../../3rdparty/libmad $$QSOUND_LIBS \
    -L../../3rdparty/qxmpp/src $$QXMPP_LIBS \
    -L../../qxmpp-extra -lqxmpp-extra

android {
    qmldir.path = /assets/imports/wiLink
    qmldir.files = qmldir
    INSTALLS += qmldir
} else:unix {
    qmldir.path = $$[QT_INSTALL_QML]/wiLink.2
    qmldir.files = qmldir
    target.path = $$[QT_INSTALL_QML]/wiLink.2
    INSTALLS += qmldir target
}

QMAKE_POST_LINK += $$QMAKE_COPY $$replace($$list($$quote($$PWD/qmldir) $$DESTDIR), /, $$QMAKE_DIR_SEP);
mac {
    QMAKE_POST_LINK += mkdir -p $$APPDIR/Contents/Frameworks;
    QMAKE_POST_LINK += cp ../../3rdparty/qxmpp/src/lib$${QXMPP_LIBRARY_NAME}.0.dylib $$APPDIR/Contents/Frameworks/;
}
