include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLite

SOURCES += \
    ../chat_client.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../chat_utils.cpp \
    flickcharm.cpp \
    main.cpp \
    window.cpp

HEADERS += \
    ../chat_client.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../chat_utils.h \
    flickcharm.h \
    window.h

RESOURCES += ../data/wiLink.qrc

INCLUDEPATH += .. $$QXMPP_INCLUDE_DIR
LIBS += $$QXMPP_LIBS
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
