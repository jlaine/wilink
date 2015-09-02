include(../../wilink.pri)

QT += network quick widgets xml

TARGET = wiLink
VERSION = $$WILINK_VERSION
DEFINES += WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"

RESOURCES += app.qrc ../data/qml.qrc
SOURCES += main.cpp network.cpp window.cpp
HEADERS += network.h window.h

mac {
    SOURCES += application_mac.mm
    LIBS += -framework AppKit
}

INCLUDEPATH += $$WILINK_INCLUDE_DIR
INCLUDEPATH += ../3rdparty/qtsingleapplication
LIBS += -L../3rdparty/qtsingleapplication -lqtsingleapplication

# Installation
QMAKE_TARGET_COMPANY="Wifirst"
QMAKE_TARGET_COPYRIGHT="Copyright (c) 2009-2013 Wifirst"
android {
} else:mac {
    ICON = ../data/wiLink.icns
    QMAKE_POST_LINK = sed -e \"s,@ICON@,wiLink.icns,g\" -e \"s,@EXECUTABLE@,wiLink,g\" -e \"s,@TYPEINFO@,????,g\" -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@COPYRIGHT@,$$QMAKE_TARGET_COPYRIGHT,g\" $$PWD/app.plist > wiLink.app/Contents/Info.plist
} else:unix {
    desktop.path = $$PREFIX/share/applications
    desktop.files = ../data/wiLink.desktop
    icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps
    icon32.files = ../data/images/32x32/wiLink.png
    icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
    icon64.files = ../data/images/64x64/wiLink.png
    pixmap.path = $$PREFIX/share/pixmaps
    pixmap.files = ../data/wiLink.xpm
    scalable.path = $$PREFIX/share/icons/hicolor/scalable/apps
    scalable.files = ../data/images/scalable/wiLink.svg
    target.path = $$PREFIX/bin
    INSTALLS += desktop icon32 icon64 pixmap scalable target
}
