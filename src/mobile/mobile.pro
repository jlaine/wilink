include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLink

DEFINES += QT_STATICPLUGIN WILINK_EMBEDDED

SOURCES += \
    ../application.cpp \
    ../chat.cpp \
    ../chat_accounts.cpp \
    ../chat_conversation.cpp \
    ../chat_client.cpp \
    ../chat_edit.cpp \
    ../chat_history.cpp \
    ../chat_panel.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../chat_search.cpp \
    ../chat_status.cpp \
    ../chat_utils.cpp \
    ../flickcharm.cpp \
    ../plugins/chats.cpp \
    ../plugins/console.cpp \
    main.cpp

HEADERS += \
    ../application.h \
    ../chat.h \
    ../chat_accounts.h \
    ../chat_client.h \
    ../chat_conversation.h \
    ../chat_edit.h \
    ../chat_history.h \
    ../chat_panel.h \
    ../chat_plugin.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../chat_search.h \
    ../chat_status.h \
    ../chat_utils.h \
    ../flickcharm.h \
    ../plugins/console.h \
    ../plugins/chats.h

mac {
    SOURCES += ../application_mac.mm
    LIBS += -framework AppKit
}

RESOURCES += ../data/wiLink.qrc

INCLUDEPATH += .. $$IDLE_INCLUDE_DIR $QNETIO_INCLUDE_DIR $$QXMPP_INCLUDE_DIR
LIBS += $$IDLE_LIBS $$QNETIO_LIBS $$QXMPP_LIBS
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

    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData"
}
