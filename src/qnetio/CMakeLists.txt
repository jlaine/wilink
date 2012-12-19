cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)
find_package(Qt4 REQUIRED)

set(PLUGIN_INSTALL_DIR lib/qt4/plugins CACHE STRING "Plugins installation directory.")

add_subdirectory(src)
if(NOT QNETIO_DONT_USE_TESTS)
    add_subdirectory(tests)
endif()
