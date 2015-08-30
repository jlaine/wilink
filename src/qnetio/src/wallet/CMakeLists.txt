set(QT_DONT_USE_QTGUI 1)
include(${QT_USE_FILE})

# needed for win32
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)

if(NOT APPLE AND NOT WIN32)

# GNOME keyring support
pkg_check_modules(GNOME_KEYRING gnome-keyring-1)
if(GNOME_KEYRING_FOUND)
  message(STATUS "Found GNOME keyring support")
  include_directories(${GNOME_KEYRING_INCLUDE_DIRS})
  add_library(qnetio-gnome SHARED gnome.cpp)
  target_link_libraries(qnetio-gnome qnetio ${GNOME_KEYRING_LIBRARIES})
  install(TARGETS qnetio-gnome
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/wallet
    RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}/wallet)
endif(GNOME_KEYRING_FOUND)

# KDE wallet support
find_package(KDE4)
if(KDE4_FOUND)
  message(STATUS "Found KWallet support")
  include_directories(${KDE4_INCLUDES})
  add_library(qnetio-kde SHARED kde.cpp)
  target_link_libraries(qnetio-kde qnetio ${KDE4_KDEUI_LIBRARY})
  install(TARGETS qnetio-kde
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/wallet
    RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}/wallet)
endif(KDE4_FOUND)

# KDE tests change the library output path, so reset it
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

endif()
