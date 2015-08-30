set(PLUGIN_INSTALL_DIR lib/qt4/plugins CACHE STRING "Plugins installation directory.")

add_subdirectory(src)
if(NOT QNETIO_DONT_USE_TESTS)
    add_subdirectory(tests)
endif()
