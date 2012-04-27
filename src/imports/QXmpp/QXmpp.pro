include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin
QT += declarative network

TARGET = qmlqxmppplugin
INCLUDEPATH += $$QXMPP_INCLUDEPATH
HEADERS += plugin.h
SOURCES += plugin.cpp
