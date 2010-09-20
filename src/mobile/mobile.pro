include(../idle/idle.pri)
include(../qnetio/qnetio.pri)
include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLite

DEFINES += QT_STATICPLUGIN WILINK_EMBEDDED

SOURCES += \
    ../application.cpp \
    ../chat.cpp \
    ../chat_accounts.cpp \
    ../chat_client.cpp \
    ../chat_history.cpp \
    ../chat_panel.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../chat_status.cpp \
    ../chat_utils.cpp \
    ../flickcharm.cpp \
    main.cpp

HEADERS += \
    ../application.h \
    ../chat.h \
    ../chat_accounts.h \
    ../chat_client.h \
    ../chat_history.h \
    ../chat_panel.h \
    ../chat_plugin.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../chat_status.h \
    ../chat_utils.h \
    ../flickcharm.h

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
