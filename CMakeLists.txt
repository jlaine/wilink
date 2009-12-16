cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(CheckIncludeFiles)
include(RelativePath)

# Project and version
project(wDesktop)
set(WDESKTOP_VERSION_MAJOR 0)
set(WDESKTOP_VERSION_MINOR 2)
set(WDESKTOP_VERSION_PATCH 2)
set(WDESKTOP_VERSION ${WDESKTOP_VERSION_MAJOR}.${WDESKTOP_VERSION_MINOR}.${WDESKTOP_VERSION_PATCH})
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

# Destination directories
set(WDESKTOP_PLUGINS_DIR lib/${PROJECT_NAME} CACHE STRING "wDesktop plugins directory.")

# Path definitions
relative_path(WDESKTOP_PLUGINS_PATH bin ${WDESKTOP_PLUGINS_DIR})

# Extra runtime dependencies
if(WIN32)
	set(WDESKTOP_EXTRA_RUNTIME
		${CMAKE_FIND_ROOT_PATH}/bin/QtCore4.dll
		${CMAKE_FIND_ROOT_PATH}/bin/QtGui4.dll
		${CMAKE_FIND_ROOT_PATH}/bin/QtNetwork4.dll
		${CMAKE_FIND_ROOT_PATH}/bin/QtXml4.dll
		${CMAKE_FIND_ROOT_PATH}/bin/mingwm10.dll
		${CMAKE_FIND_ROOT_PATH}/bin/libeay32.dll
		${CMAKE_FIND_ROOT_PATH}/bin/ssleay32.dll
		${CMAKE_FIND_ROOT_PATH}/bin/libz.dll)
	set(WDESKTOP_EXTRA_PLUGINS
		${QT_PLUGINS_DIR}/imageformats/qgif4.dll
		${QT_PLUGINS_DIR}/imageformats/qjpeg4.dll)
    install(FILES ${WDESKTOP_EXTRA_RUNTIME} DESTINATION bin)
    install(FILES ${WDESKTOP_EXTRA_PLUGINS} DESTINATION ${WDESKTOP_PLUGINS_DIR}/imageformats)
endif(WIN32)

# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "wDesktop")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_PACKAGE_EXECUTABLES "wDesktop" "wDesktop")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "wDesktop")
set(CPACK_PACKAGE_VERSION_MAJOR ${WDESKTOP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${WDESKTOP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${WDESKTOP_VERSION_PATCH})
if(APPLE)
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/src/data/wDesktop.icns)
	set(CPACK_BUNDLE_IDENTIFIER net.wifirst.wDesktop)
	set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/Bundle.plist)
	set(CPACK_BUNDLE_STARTUP_COMMAND ${CMAKE_BINARY_DIR}/Bundle.start)
endif(APPLE)
if(WIN32)
    set(CPACK_NSIS_PAGE_COMPONENTS "!define MUI_FINISHPAGE_RUN \\\"$INSTDIR\\\\bin\\\\wDesktop.exe\\\";")
endif(WIN32)
include(CPack)
configure_file(${CMAKE_SOURCE_DIR}/cmake/Bundle.start.in ${CMAKE_BINARY_DIR}/Bundle.start)
configure_file(${CMAKE_SOURCE_DIR}/cmake/Bundle.plist.in ${CMAKE_BINARY_DIR}/Bundle.plist)

add_subdirectory(src)
