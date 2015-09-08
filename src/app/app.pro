include(../../wilink.pri)

QT += multimedia network quick widgets xml

TARGET = wiLink
DESTDIR = $$WILINK_APP_PATH
VERSION = $$WILINK_VERSION
DEFINES += WILINK_VERSION=\\\"$${WILINK_VERSION}\\\"

RESOURCES += app.qrc ../data/qml.qrc
SOURCES += main.cpp network.cpp qtlocalpeer.cpp
HEADERS += network.h qtlocalpeer.h

mac {
    OBJECTIVE_SOURCES += application_mac.mm
    LIBS += -framework AppKit
    QT += webkitwidgets
}

# Installation
QMAKE_TARGET_COMPANY="Wifirst"
QMAKE_TARGET_COPYRIGHT="Copyright (c) 2009-2015 Wifirst"
android {
} else:mac {
    ICON = ../data/wiLink.icns
    QMAKE_POST_LINK += sed -e \"s,@ICON@,wiLink.icns,g\" -e \"s,@EXECUTABLE@,wiLink,g\" -e \"s,@TYPEINFO@,????,g\" -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@COPYRIGHT@,$$QMAKE_TARGET_COPYRIGHT,g\" $$PWD/app.plist > $$DESTDIR/wiLink.app/Contents/Info.plist;
    QMAKE_POST_LINK += mkdir -p $$WILINK_QML_PATH/QtQuick;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Controls $$WILINK_QML_PATH/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Layouts $$WILINK_QML_PATH/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/LocalStorage $$WILINK_QML_PATH/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/Window.2 $$WILINK_QML_PATH/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick/XmlListModel $$WILINK_QML_PATH/QtQuick/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtQuick.2 $$WILINK_QML_PATH/;
    QMAKE_POST_LINK += cp -r $$[QT_INSTALL_QML]/QtWebkit $$WILINK_QML_PATH/;

    # QtWebProcess
    WEBPROCESS_DIR = $$DESTDIR/wiLink.app/Contents/libexec
    WEBPROCESS_PATH = $$WEBPROCESS_DIR/QtWebProcess
    QMAKE_POST_LINK += mkdir -p $$DESTDIR/wiLink.app/Contents/libexec;
    QMAKE_POST_LINK += cp $$[QT_INSTALL_LIBEXECS]/QtWebProcess $$WEBPROCESS_DIR;
    QMAKE_POST_LINK += install_name_tool -delete_rpath /work/build/qt5_workdir/w/s/qtwebkit/lib $$WEBPROCESS_PATH;
    QMAKE_POST_LINK += install_name_tool -delete_rpath @loader_path/../lib $$WEBPROCESS_PATH;
    QMAKE_POST_LINK += install_name_tool -add_rpath @executable_path/../Frameworks $$WEBPROCESS_PATH;
    QMAKE_POST_LINK += (echo '[Paths]'; echo 'Plugins = ../PlugIns') > $$WEBPROCESS_DIR/qt.conf;

    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/macdeployqt $$DESTDIR/wiLink.app -qmldir=$$WILINK_SOURCE_TREE/src/data/qml;
} else:win32 {
    RC_ICONS = ../data/wiLink.ico
    SSL_DIR = $$[QT_INSTALL_PREFIX]/../../Tools/QtCreator/bin
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$SSL_DIR/libeay32.dll)) $$shell_quote($$shell_path($$DESTDIR)) $$EOL
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$SSL_DIR/ssleay32.dll)) $$shell_quote($$shell_path($$DESTDIR)) $$EOL
    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/windeployqt --qmldir $$WILINK_SOURCE_TREE/src/data/qml --webkit2 $$DESTDIR/wiLink.exe
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
