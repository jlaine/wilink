cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(CheckIncludeFiles)

# Project and version
project(wiLink)
set(WILINK_VERSION_MAJOR 0)
set(WILINK_VERSION_MINOR 7)
set(WILINK_VERSION_PATCH 910)
set(WILINK_VERSION ${WILINK_VERSION_MAJOR}.${WILINK_VERSION_MINOR}.${WILINK_VERSION_PATCH})
configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in ${CMAKE_BINARY_DIR}/src/config.h)

# Required
find_package(Qt4 REQUIRED)
if(UNIX AND NOT APPLE)
    set(QT_USE_QTDBUS true)
endif(UNIX AND NOT APPLE)

# Wireless
find_library(COREWLAN_LIBRARY NAMES CoreWLAN)
if(COREWLAN_LIBRARY)
    message(STATUS "Found CoreWLAN library: ${COREWLAN_LIBRARY}")
    add_definitions(-DUSE_COREWLAN)
    set(WLAN_LIBRARIES ${COREWLAN_LIBRARY})
endif(COREWLAN_LIBRARY)

# Extra runtime dependencies
if(APPLE)
    set(WILINK_EXTRA_PLUGINS
        ${QT_PLUGINS_DIR}/imageformats/libqgif.*
        ${QT_PLUGINS_DIR}/imageformats/libqjpeg.*
        ${QT_PLUGINS_DIR}/sqldrivers/libqsqlite.*)
endif(APPLE)
if(WIN32)
    set(WILINK_EXTRA_RUNTIME
        ${CMAKE_FIND_ROOT_PATH}/bin/QtCore4.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtGui4.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtNetwork4.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtSql4.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/QtXml4.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/mingwm10.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libeay32.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libgcc_s_dw2-1.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/ssleay32.dll
        ${CMAKE_FIND_ROOT_PATH}/bin/libz.dll)
    set(WILINK_IMAGE_PLUGINS
        ${QT_PLUGINS_DIR}/imageformats/qgif4.dll
        ${QT_PLUGINS_DIR}/imageformats/qjpeg4.dll)
    set(WILINK_SQL_PLUGINS
        ${QT_PLUGINS_DIR}/sqldrivers/qsqlite4.dll)
    install(FILES ${WILINK_EXTRA_RUNTIME} DESTINATION bin)
    install(FILES ${WILINK_IMAGE_PLUGINS} DESTINATION bin/imageformats)
    install(FILES ${WILINK_SQL_PLUGINS} DESTINATION bin/sqldrivers)
endif(WIN32)

# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "wiLink")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_PACKAGE_EXECUTABLES "wiLink" "wiLink")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "wiLink")
set(CPACK_PACKAGE_VERSION_MAJOR ${WILINK_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${WILINK_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${WILINK_VERSION_PATCH})
# Binary package
if(APPLE)
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/src/data/wiLink.icns)
	set(CPACK_BUNDLE_IDENTIFIER net.wifirst.wiLink)

    # image generation
    execute_process(COMMAND sw_vers -productVersion COMMAND cut -d. -f1,2
        OUTPUT_VARIABLE OSX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(BUNDLE_DMG ${PROJECT_NAME}-${WILINK_VERSION}-mac-${OSX_VERSION}.dmg)
    set(BUNDLE_LOCATION ${CMAKE_BINARY_DIR}/src/wiLink.app)
    set(_dmg_temp ${BUNDLE_DMG}.temp)
    add_custom_command(OUTPUT ${BUNDLE_DMG}
        COMMAND mkdir ${_dmg_temp}
        COMMAND cp -r ${BUNDLE_LOCATION} ${_dmg_temp}
        COMMAND ln -s /Applications ${_dmg_temp}/Applications
        COMMAND hdiutil create ${BUNDLE_DMG} -srcdir ${_dmg_temp} -fs HFS+ -volname "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${WILINK_VERSION}"
        COMMAND rm -rf ${_dmg_temp}
        DEPENDS wiLink)
    add_custom_target(package DEPENDS ${BUNDLE_DMG})
else(APPLE)
    if(WIN32)
        set(CPACK_NSIS_PAGE_COMPONENTS "!define MUI_FINISHPAGE_RUN \\\"$INSTDIR\\\\bin\\\\wiLink.exe\\\";")
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
