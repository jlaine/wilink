include(../../wilink.pri)

QT += declarative network sql xml

TARGET = wiLink
VERSION = $$WILINK_VERSION

# workaround for QTBUG-19232
symbian {
    DEFINES += WILINK_VERSION=\"$${WILINK_VERSION}\"
} else {
    DEFINES += WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"
}

# FIXME: this is a hack so that Q_OS_ANDROID is defined
android {
    DEFINES += ANDROID
}
contains(MEEGO_EDITION,harmattan) {
    DEFINES += MEEGO_EDITION_HARMATTAN
}

# embedded version
android|symbian|contains(MEEGO_EDITION,harmattan) {
    DEFINES += WILINK_EMBEDDED
}

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
QMAKE_TARGET_COPYRIGHT="Copyright (c) 2009-2012 Wifirst"
android {
} else:mac {
    ICON = ../data/wiLink.icns
    QMAKE_POST_LINK = sed -e \"s,@ICON@,wiLink.icns,g\" -e \"s,@EXECUTABLE@,wiLink,g\" -e \"s,@TYPEINFO@,????,g\" -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@COPYRIGHT@,$$QMAKE_TARGET_COPYRIGHT,g\" $$PWD/app.plist > wiLink.app/Contents/Info.plist
} else:symbian {
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"$$QMAKE_TARGET_COMPANY\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"$$QMAKE_TARGET_COMPANY\"" \
        " "

    mobile_deployment.pkg_prerules += vendorinfo
    DEPLOYMENT += mobile_deployment

    ICON = ../data/scalable/wiLink.svg

    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData UserEnvironment"
} else:unix {
    QT += dbus
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
