include(../../../wilink.pri)
include(diagnostics/diagnostics.pri)
include(idle/idle.pri)
include(sound/sound.pri)
include(wallet/wallet.pri)

TEMPLATE = lib
CONFIG += qt plugin

QT += multimedia network quick widgets xml

DESTDIR = ../../app/wiLink.2
TARGET = qmlwilinkplugin

android {
    DEFINES += ANDROID
    DEFINES += WILINK_EMBEDDED
} else:mac {
    APPDIR = ../../app/wiLink.app
    DESTDIR = $$APPDIR/Contents/Resources/qml/wiLink.2
    OBJECTIVE_SOURCES += settings_mac.mm
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

INCLUDEPATH += $$WILINK_INCLUDE_DIR

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

!isEmpty(WILINK_SYSTEM_QXMPP) {
    INCLUDEPATH += /usr/include/qxmpp
    LIBS += -lqxmpp
} else {
    include(../../3rdparty/qxmpp/qxmpp.pri)
    INCLUDEPATH += $$QXMPP_INCLUDEPATH
    LIBS += -L../../3rdparty/qxmpp/src $$QXMPP_LIBS
    mac {
        QMAKE_POST_LINK += mkdir -p $$APPDIR/Contents/Frameworks;
        QMAKE_POST_LINK += cp ../../3rdparty/qxmpp/src/lib$${QXMPP_LIBRARY_NAME}.0.dylib $$APPDIR/Contents/Frameworks/;
    }
}
