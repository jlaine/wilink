include(../../wilink.pri)

QT += multimedia network quick widgets xml

TARGET = wiLink
VERSION = $$WILINK_VERSION
DEFINES += WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"

RESOURCES += app.qrc ../data/qml.qrc
SOURCES += main.cpp network.cpp qtlocalpeer.cpp
HEADERS += network.h qtlocalpeer.h

mac {
    OBJECTIVE_SOURCES += application_mac.mm
    LIBS += -framework AppKit
}

# Installation
QMAKE_TARGET_COMPANY="Wifirst"
QMAKE_TARGET_COPYRIGHT="Copyright (c) 2009-2015 Wifirst"
android {
} else:mac {
    ICON = ../data/wiLink.icns
    QMAKE_POST_LINK += sed -e \"s,@ICON@,wiLink.icns,g\" -e \"s,@EXECUTABLE@,wiLink,g\" -e \"s,@TYPEINFO@,????,g\" -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@COPYRIGHT@,$$QMAKE_TARGET_COPYRIGHT,g\" $$PWD/app.plist > wiLink.app/Contents/Info.plist;
    QMAKE_POST_LINK += mkdir -p wiLink.app/Contents/Resources/qml/QtQuick;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Controls wiLink.app/Contents/Resources/qml/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Layouts wiLink.app/Contents/Resources/qml/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/LocalStorage wiLink.app/Contents/Resources/qml/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Window.2 wiLink.app/Contents/Resources/qml/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/XmlListModel wiLink.app/Contents/Resources/qml/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick.2 wiLink.app/Contents/Resources/qml/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtWebkit wiLink.app/Contents/Resources/qml/;
    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/macdeployqt wiLink.app
} else:unix {
    desktop.path = $$PREFIX/share/applications
    desktop.files = ../data/wiLink.desktop
    icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps
    icon32.files = ../data/images/32x32/wiLink.png
    icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
    icon64.files = ../data/images/64x64/wiLink.png
    icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps
    icon128.files = ../data/images/128x128/wiLink.png
    icon256.path = $$PREFIX/share/icons/hicolor/256x256/apps
    icon256.files = ../data/images/256x256/wiLink.png
    scalable.path = $$PREFIX/share/icons/hicolor/scalable/apps
    scalable.files = ../data/images/scalable/wiLink.svg
    target.path = $$PREFIX/bin
    INSTALLS += desktop icon32 icon64 icon128 icon256 scalable target
}
