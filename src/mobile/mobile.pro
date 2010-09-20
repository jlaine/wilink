include(../qxmpp/qxmpp.pri)

QT += network xml

TARGET = wiLite

SOURCES += main.cpp

INCLUDEPATH += .. $$QXMPP_INCLUDE_DIR
LIBS += $$QXMPP_LIBS
PRE_TARGETDEPS += $$QXMPP_LIBRARY_FILE
