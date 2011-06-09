include(../src.pri)
include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qsound/qsound.pri)
include(../qxmpp/qxmpp.pri)

QT += declarative network xml

TARGET = wiLink
VERSION = 1.1.4

DEFINES += QT_STATICPLUGIN WILINK_EMBEDDED

SOURCES += \
    ../application.cpp \
    ../chat.cpp \
    ../chat_accounts.cpp \
    ../chat_client.cpp \
    ../chat_form.cpp \
    ../chat_history.cpp \
    ../chat_model.cpp \
    ../chat_preferences.cpp \
    ../chat_utils.cpp \
    ../systeminfo.cpp \
    ../updates.cpp \
    ../updatesdialog.cpp \
    ../plugins/calls.cpp \
    ../plugins/console.cpp \
    ../plugins/conversations.cpp \
    ../plugins/declarative.cpp \
    ../plugins/diagnostics.cpp \
    ../plugins/discovery.cpp \
    ../plugins/phone.cpp \
    ../plugins/phone/sip.cpp \
    ../plugins/photos.cpp \
    ../plugins/player.cpp \
    ../plugins/rooms.cpp \
    ../plugins/roster.cpp \
    ../plugins/shares.cpp \
    main.cpp

HEADERS += \
    ../application.h \
    ../chat.h \
    ../chat_accounts.h \
    ../chat_client.h \
    ../chat_form.h \
    ../chat_history.h \
    ../chat_model.h \
    ../chat_preferences.h \
    ../chat_utils.h \
    ../systeminfo.h \
    ../updates.h \
    ../updatesdialog.h \
    ../plugins/calls.h \
    ../plugins/console.h \
    ../plugins/conversations.h \
    ../plugins/declarative.h \
    ../plugins/diagnostics.h \
    ../plugins/discovery.h \
    ../plugins/photos.h \
    ../plugins/phone.h \
    ../plugins/player.h \
    ../plugins/phone/sip.h \
    ../plugins/phone/sip_p.h \
    ../plugins/rooms.h \
    ../plugins/roster.h \
    ../plugins/shares.h \
    config.h

mac {
    SOURCES += ../application_mac.mm
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
