include(../../../wilink.pri)
include(diagnostics/diagnostics.pri)
include(idle/idle.pri)
include(sound/sound.pri)
include(wallet/wallet.pri)

TEMPLATE = lib
CONFIG += qt plugin
QT += multimedia network quick widgets xml

TARGET = qmlwilinkplugin
DESTDIR = $$WILINK_QML_PATH/wiLink.2

android {
    DEFINES += WILINK_EMBEDDED
} else:mac {
    OBJECTIVE_SOURCES += settings_mac.mm
} else {
    DEFINES += USE_SYSTRAY
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
    phone/sip.h \
    phone/sip_p.h \
    rooms.h \
    roster.h \
    settings.h \
    systeminfo.h \
    translations.h \
    updater.h

RESOURCES += ../../data/wiLink.qrc

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

QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$shell_quote($$PWD/qmldir) $$shell_quote($$DESTDIR)) $$EOL

!isEmpty(WILINK_SYSTEM_QXMPP) {
    INCLUDEPATH += /usr/include/qxmpp
    LIBS += -lqxmpp
} else {
    include(../../3rdparty/qxmpp/qxmpp.pri)
    QXMPP_LIBRARY_DIR = ../../3rdparty/qxmpp/src
    mac {
        QXMPP_LIBRARY_FILE = lib$${QXMPP_LIBRARY_NAME}.0.dylib
    } else:win32 {
        QXMPP_LIBRARY_FILE = $${QXMPP_LIBRARY_NAME}0.dll
    } else:unix {
        QXMPP_LIBRARY_FILE = lib$${QXMPP_LIBRARY_NAME}.so.0
    }

    INCLUDEPATH += $$QXMPP_INCLUDEPATH
    LIBS += -L$$QXMPP_LIBRARY_DIR $$QXMPP_LIBS

    QMAKE_POST_LINK += $$sprintf($$QMAKE_MKDIR_CMD, $$shell_path($$shell_quote($$WILINK_LIB_PATH))) $$EOL
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$shell_quote($$QXMPP_LIBRARY_DIR/$$QXMPP_LIBRARY_FILE) $$shell_quote($$WILINK_LIB_PATH))
}
