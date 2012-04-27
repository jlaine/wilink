include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin
QT += declarative network

TARGET = qmlqxmppplugin
INCLUDEPATH += $$QXMPP_INCLUDEPATH
LIBS += -L../../3rdparty/qxmpp/src $$QXMPP_LIBS
HEADERS += plugin.h
SOURCES += plugin.cpp
