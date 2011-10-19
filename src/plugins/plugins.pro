include(../../wilink.pri)
include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qsound/qsound.pri)
include(../qxmpp/qxmpp.pri)

QT += declarative network xml

TARGET = wiLink
VERSION = $$WILINK_VERSION

DEFINES += QT_STATICPLUGIN

# workaround for QTBUG-19232
symbian {
    DEFINES += WILINK_VERSION=\"$${WILINK_VERSION}\"
} else {
    DEFINES += WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"
}

# FIXME: this is a hack so that Q_OS_ANDROID is defined
android {
    DEFINES += ANDROID
}

# embedded version
android|symbian|contains(MEEGO_EDITION,harmattan) {
    DEFINES += WILINK_EMBEDDED
}

SOURCES += \
    application.cpp \
    calls.cpp \
    client.cpp \
    console.cpp \
    conversations.cpp \
    declarative.cpp \
    diagnostics.cpp \
    discovery.cpp \
    history.cpp \
    main.cpp \
    model.cpp \
    phone.cpp \
    phone/sip.cpp \
    photos.cpp \
    player.cpp \
    rooms.cpp \
    roster.cpp \
    shares.cpp \
    systeminfo.cpp \
    updater.cpp \
    window.cpp

HEADERS += \
    application.h \
    calls.h \
    client.h \
    console.h \
    conversations.h \
    declarative.h \
    diagnostics.h \
    discovery.h \
    history.h \
    model.h \
    photos.h \
    phone.h \
    player.h \
    phone/sip.h \
    phone/sip_p.h \
    rooms.h \
    roster.h \
    shares.h \
    systeminfo.h \
    updater.h \
    window.h \

mac {
    SOURCES += application_mac.mm
    LIBS += -framework AppKit
}

RESOURCES += ../data/wiLink.qrc

INCLUDEPATH += \
    $$WILINK_INCLUDE_DIR \
    $$IDLE_INCLUDE_DIR \
    $$QNETIO_INCLUDE_DIR \
    $$QSOUND_INCLUDE_DIR \
    $$QXMPP_INCLUDE_DIR
LIBS += \
    $$IDLE_LIBS \
    $$QNETIO_LIBS \
    $$QSOUND_LIBS \
    $$QXMPP_LIBS
PRE_TARGETDEPS += $$QXMPP_LIBRARY_FILE

# Installation
isEmpty(PREFIX) {
    PREFIX=/usr
}
desktop.path = $$PREFIX/share/applications
desktop.files = ../data/wiLink.desktop
icon.path = $$PREFIX/share/icons/hicolor/32x32/apps
icon.files = ../data/wiLink.png
pixmap.path = $$PREFIX/share/pixmaps
pixmap.files = ../data/wiLink.xpm
scalable.path = $$PREFIX/share/icons/hicolor/scalable/apps
scalable.files = ../data/wiLink.svg
target.path = $$PREFIX/bin
INSTALLS += desktop icon pixmap scalable target

# Symbian packaging rules
mac {
    ICON = ../data/wiLink.icns
    QMAKE_INFO_PLIST = ../data/wiLink.plist
} else:symbian {
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Wifirst\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Wifirst\"" \
        " "

    mobile_deployment.pkg_prerules += vendorinfo
    DEPLOYMENT += mobile_deployment

    ICON = ../data/wiLink.svg

    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData UserEnvironment"
}
