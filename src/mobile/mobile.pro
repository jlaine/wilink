include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLite

SOURCES += \
    ../chat_client.cpp \
    ../chat_roster.cpp \
    ../chat_roster_item.cpp \
    ../utils.cpp \
    main.cpp \
    window.cpp

HEADERS += \
    ../chat_client.h \
    ../chat_roster.h \
    ../chat_roster_item.h \
    ../utils.h \
    window.h

INCLUDEPATH += .. $$QXMPP_INCLUDE_DIR
LIBS += $$QXMPP_LIBS
PRE_TARGETDEPS += $$QXMPP_LIBRARY_FILE
