include(../src.pri)
include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qsound/qsound.pri)
include(../qxmpp/qxmpp.pri)

QT += declarative network xml

TARGET = wiLink
VERSION = $$WILINK_VERSION

DEFINES += QT_STATICPLUGIN WILINK_EMBEDDED WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"

SOURCES += \
    application.cpp \
    accounts.cpp \
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
    preferences.cpp \
    rooms.cpp \
    roster.cpp \
    shares.cpp \
    shares/options.cpp \
    systeminfo.cpp \
    updates.cpp \
    updatesdialog.cpp \
    utils.cpp \
    window.cpp

HEADERS += \
    accounts.h \
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
    preferences.h \
    rooms.h \
    roster.h \
    shares.h \
    shares/options.h \
    systeminfo.h \
    updates.h \
    updatesdialog.h \
    utils.h \
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

# Symbian packaging rules
symbian {
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
