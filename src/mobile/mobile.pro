include(../src.pri)
include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qsound/qsound.pri)
include(../qxmpp/qxmpp.pri)

QT += multimedia network xml

TARGET = wiLink
VERSION = 1.0.8

DEFINES += QT_STATICPLUGIN WILINK_EMBEDDED

SOURCES += \
    ../application.cpp \
    ../chat.cpp \
    ../chat_accounts.cpp \
    ../chat_conversation.cpp \
    ../chat_client.cpp \
    ../chat_edit.cpp \
    ../chat_form.cpp \
    ../chat_history.cpp \
    ../chat_panel.cpp \
    ../chat_preferences.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../chat_search.cpp \
    ../chat_status.cpp \
    ../chat_utils.cpp \
    ../flickcharm.cpp \
    ../systeminfo.cpp \
    ../updates.cpp \
    ../updatesdialog.cpp \
    ../plugins/calls.cpp \
    ../plugins/chats.cpp \
    ../plugins/console.cpp \
    ../plugins/contacts.cpp \
    ../plugins/phone.cpp \
    ../plugins/phone/models.cpp \
    ../plugins/phone/sip.cpp \
    ../plugins/photos.cpp \
    ../plugins/rooms.cpp \
    ../plugins/transfers.cpp \
    main.cpp

HEADERS += \
    ../application.h \
    ../chat.h \
    ../chat_accounts.h \
    ../chat_client.h \
    ../chat_conversation.h \
    ../chat_edit.h \
    ../chat_form.h \
    ../chat_history.h \
    ../chat_panel.h \
    ../chat_plugin.h \
    ../chat_preferences.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../chat_search.h \
    ../chat_status.h \
    ../chat_utils.h \
    ../flickcharm.h \
    ../systeminfo.h \
    ../updates.h \
    ../updatesdialog.h \
    ../plugins/calls.h \
    ../plugins/chats.h \
    ../plugins/contacts.h \
    ../plugins/console.h \
    ../plugins/photos.h \
    ../plugins/phone.h \
    ../plugins/phone/models.h \
    ../plugins/phone/sip.h \
    ../plugins/rooms.h \
    ../plugins/transfers.h

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
