include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLite

SOURCES += \
    ../chat_client.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../chat_utils.cpp \
    main.cpp \
    window.cpp

HEADERS += \
    ../chat_client.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../chat_utils.h \
    window.h

RESOURCES += ../data/wiLink.qrc

INCLUDEPATH += .. $$QXMPP_INCLUDE_DIR
LIBS += $$QXMPP_LIBS
PRE_TARGETDEPS += $$QXMPP_LIBRARY_FILE
