cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(CheckIncludeFiles)
include(RelativePath)
include(ShipDepends)

# Project and version
project(wDesktop)
set(WDESKTOP_VERSION_MAJOR 0)
set(WDESKTOP_VERSION_MINOR 1)
set(WDESKTOP_VERSION_PATCH 0)
set(WDESKTOP_VERSION ${WDESKTOP_VERSION_MAJOR}.${WDESKTOP_VERSION_MINOR}.${WDESKTOP_VERSION_PATCH})

# Required
find_package(Qt4 REQUIRED)

# Destination directories
set(WDESKTOP_PLUGINS_DIR lib/${PROJECT_NAME} CACHE STRING "wDesktop plugins directory.")

# Path definitions
relative_path(WDESKTOP_PLUGINS_PATH bin ${WDESKTOP_PLUGINS_DIR})

# Extra runtime dependencies
if(APPLE)
	set(WDESKTOP_EXTRA_PLUGINS
		${QT_PLUGINS_DIR}/imageformats/libqgif.bundle
		${QT_PLUGINS_DIR}/imageformats/libqjpeg.bundle)
endif(APPLE)
if(WIN32)
	set(WDESKTOP_EXTRA_RUNTIME
		${CMAKE_FIND_ROOT_PATH}/bin/libeay32.dll
		${CMAKE_FIND_ROOT_PATH}/bin/ssleay32.dll
		${CMAKE_FIND_ROOT_PATH}/bin/libz.dll)
	set(WDESKTOP_EXTRA_PLUGINS
		${QT_PLUGINS_DIR}/imageformats/qgif4.dll
		${QT_PLUGINS_DIR}/imageformats/qjpeg4.dll)
endif(WIN32)
install(FILES ${WDESKTOP_EXTRA_RUNTIME} DESTINATION bin)
install(FILES ${WDESKTOP_EXTRA_PLUGINS} DESTINATION ${WDESKTOP_PLUGINS_DIR}/imageformats)

# Packaging
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "wDesktop")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_PACKAGE_EXECUTABLES "wDesktop" "wDesktop")
set(CPACK_PACKAGE_VERSION_MAJOR ${WDESKTOP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${WDESKTOP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${WDESKTOP_VERSION_PATCH})
if(APPLE)
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/data/wDesktop.icns)
	set(CPACK_BUNDLE_IDENTIFIER net.wifirst.wDesktop)
	set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/Bundle.plist)
	set(CPACK_BUNDLE_STARTUP_COMMAND ${CMAKE_BINARY_DIR}/Bundle.start)
endif(APPLE)
include(CPack)
configure_file(${CMAKE_SOURCE_DIR}/cmake/Bundle.start.in ${CMAKE_BINARY_DIR}/Bundle.start)
configure_file(${CMAKE_SOURCE_DIR}/cmake/Bundle.plist.in ${CMAKE_BINARY_DIR}/Bundle.plist)

add_subdirectory(data)
add_subdirectory(src)
