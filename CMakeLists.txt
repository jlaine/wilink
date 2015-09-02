cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
include(CheckIncludeFiles)

# Project and version
project(wiLink)
set(WILINK_VERSION_MAJOR 2)
set(WILINK_VERSION_MINOR 4)
set(WILINK_VERSION_PATCH 90)
set(WILINK_VERSION ${WILINK_VERSION_MAJOR}.${WILINK_VERSION_MINOR}.${WILINK_VERSION_PATCH})

# Required
find_package(Qt5DBus)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5X11Extras)
find_package(Qt5Xml REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(QXMPP qxmpp REQUIRED)

# Extra runtime dependencies
get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)
execute_process(
    COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_QML
    OUTPUT_VARIABLE QT_INSTALL_QML
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(WILINK_IMPORTS_DIR ${QT_INSTALL_QML})
if(APPLE)
    set(WILINK_EXTRA_PLUGINS
        imageformats/libqgif.dylib
        imageformats/libqjpeg.dylib
        sqldrivers/libqsqlite.dylib)
endif(APPLE)
if(WIN32)
    set(WILINK_IMPORTS_DIR bin)
    install(FILES
        ${CMAKE_FIND_ROOT_PATH}/bin/QtCore5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtGui5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtMultimedia5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtNetwork5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtQml5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtQuick5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtWebKit5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtWidgets5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtXml5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtXmlPatterns5.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/mingwm10.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libeay32.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libgcc_s_dw2-1.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libogg-0.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libspeex-1.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libvorbis-0.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libvorbisfile-3.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libtheoradec-1.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libtheoraenc-1.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libz.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/qxmpp0.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/ssleay32.dll
        DESTINATION bin)
    install(FILES
        ${QT_PLUGINS_DIR}/imageformats/qgif5.dll
        ${QT_PLUGINS_DIR}/imageformats/qjpeg5.dll
        DESTINATION bin/imageformats)
    install(FILES
        ${QT_PLUGINS_DIR}/sqldrivers/qsqlite5.dll
        DESTINATION bin/sqldrivers)
    install(FILES
        ${QT_IMPORTS_DIR}/QtWebKit/qmldir
        ${QT_IMPORTS_DIR}/QtWebKit/qmlwebkitplugin.dll
        DESTINATION ${WILINK_IMPORTS_DIR}/QtWebKit)
endif(WIN32)

# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "wiLink")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_PACKAGE_EXECUTABLES "wiLink" "wiLink")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "wiLink")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "wiLink")
set(CPACK_PACKAGE_VERSION_MAJOR ${WILINK_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${WILINK_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${WILINK_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "Wifirst")

# Binary package
if(APPLE)
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/src/data/wiLink.icns)
	set(CPACK_BUNDLE_IDENTIFIER net.wifirst.wiLink)

    # image generation
    execute_process(COMMAND sw_vers -productVersion COMMAND cut -d. -f1,2
        OUTPUT_VARIABLE OSX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(BUNDLE_DMG ${PROJECT_NAME}-${WILINK_VERSION}-mac-${OSX_VERSION}.dmg)
    set(BUNDLE_LOCATION ${CMAKE_BINARY_DIR}/src/app/wiLink.app)
    set(_dmg_temp ${BUNDLE_DMG}.temp)
    add_custom_command(OUTPUT ${BUNDLE_DMG}
        COMMAND rm -rf ${_dmg_temp} ${BUNDLE_DMG}
        COMMAND mkdir ${_dmg_temp}
        COMMAND cp -r ${BUNDLE_LOCATION} ${_dmg_temp}
        COMMAND ln -s /Applications ${_dmg_temp}/Applications
        COMMAND hdiutil create ${BUNDLE_DMG} -srcdir ${_dmg_temp} -fs HFS+ -volname "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${WILINK_VERSION}"
        COMMAND rm -rf ${_dmg_temp}
        DEPENDS wiLink)
    add_custom_target(package DEPENDS ${BUNDLE_DMG})
else(APPLE)
    if(WIN32)
        set(CPACK_NSIS_DEFINES "
  VIProductVersion ${WILINK_VERSION}.0
  VIAddVersionKey CompanyName \\\"Wifirst\\\"
  VIAddVersionKey FileDescription \\\"wiLink ${WILINK_VERSION}\\\"
  VIAddVersionKey FileVersion \\\"${WILINK_VERSION}\\\"
  VIAddVersionKey LegalCopyright \\\"Copyright 2009-2013 Wifirst\\\"
  VIAddVersionKey ProductName \\\"wiLink\\\"
  VIAddVersionKey ProductVersion \\\"${WILINK_VERSION}\\\"")
        set(CPACK_NSIS_PAGE_COMPONENTS "!define MUI_FINISHPAGE_RUN \\\"$INSTDIR\\\\bin\\\\wiLink.exe\\\";")
        set(CPACK_NSIS_FULL_INSTALL "
  ;Make sure program is closed
  ExecWait '\\\"taskkill.exe\\\" /F /IM wiLink.exe' \\\$0
  Sleep 1000

  ;Remove old registry keys
  DeleteRegKey SHCTX \\\"Software\\\\Humanity\\\\wiLink 1.1.0\\\"
  DeleteRegKey SHCTX \\\"Software\\\\Humanity\\\\wiLink 2.0.1\\\"
  DeleteRegKey SHCTX \\\"Software\\\\Humanity\\\\wiLink 2.1.0\\\"
  DeleteRegKey SHCTX \\\"Software\\\\Wifirst\\\\wiLink 2.1.901\\\"
  DeleteRegKey SHCTX \\\"Software\\\\Wifirst\\\\wiLink 2.1.902\\\"

  ;Remove old files
  File /r \\\"\\\${INST_DIR}\\\\*.*\\\"")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  ;Add protocol handler
  WriteRegStr HKCR \\\"wilink\\\" \\\"\\\" \\\"URL:wilink\\\"
  WriteRegStr HKCR \\\"wilink\\\" \\\"URL Protocol\\\" \\\"\\\"
  WriteRegStr HKCR \\\"wilink\\\\shell\\\" \\\"\\\" \\\"\\\"
  WriteRegStr HKCR \\\"wilink\\\\shell\\\\open\\\" \\\"\\\" \\\"\\\"
  WriteRegStr HKCR \\\"wilink\\\\shell\\\\open\\\\command\\\" \\\"\\\" \\\"$INSTDIR\\\\bin\\\\wiLink.exe %1\\\"

  ;In silent mode, automatically run program
  IfSilent 0 +2
  Exec \\\"$INSTDIR\\\\bin\\\\wiLink.exe\\\"")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  ;Purge program data
  RMDir /r \\\"$LOCALAPPDATA\\\\Wifirst\\\\wiLink\\\"
  RMDir \\\"$LOCALAPPDATA\\\\Wifirst\\\"
  DeleteRegValue HKCU \\\"Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Run\\\" \\\"wiLink\\\"
  DeleteRegKey HKCU \\\"Software\\\\Wifirst\\\\wiLink\\\"
  DeleteRegKey /ifempty HKCU \\\"Software\\\\Wifirst\\\"

  ; Remove protocol handler
  DeleteRegKey HKCR \\\"wilink\\\"")
    endif(WIN32)
    # Source package
    set(CPACK_SOURCE_GENERATOR "TBZ2")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${WILINK_VERSION}")
    list(APPEND CPACK_SOURCE_IGNORE_FILES "/.git/")
    list(APPEND CPACK_SOURCE_IGNORE_FILES "/.gitmodules")
    list(APPEND CPACK_SOURCE_IGNORE_FILES "${CMAKE_CURRENT_BINARY_DIR}")
    include(CPack)
endif(APPLE)

add_subdirectory(src)
