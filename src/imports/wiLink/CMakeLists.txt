#include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qnetio/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qsound/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qxmpp-extra/diagnostics)

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

if(WILINK_EMBEDDED)
    add_definitions(-DWILINK_EMBEDDED)
endif()
if(NOT APPLE)
    add_definitions(-DUSE_SYSTRAY)
endif(NOT APPLE)

set(plugin_SOURCES
    accounts.cpp
    calls.cpp
    client.cpp
    console.cpp
    conversations.cpp
    declarative.cpp
    declarative_qxmpp.h
    diagnostics.cpp
    discovery.cpp
    history.cpp
    icons.cpp
    idle/idle.cpp
    menubar.cpp
    model.cpp
    notifications.cpp
    phone.cpp
    phone/sip.cpp
    places.cpp
    rooms.cpp
    roster.cpp
    settings.cpp
    systeminfo.cpp
    translations.cpp
    updater.cpp)

if(APPLE)
    # Growl support
    find_library(GROWL_LIBRARY Growl)
    if(GROWL_LIBRARY)
        message(STATUS "Found Growl library")
        add_definitions(-DUSE_GROWL)
        list(APPEND plugin_LIBRARIES ${GROWL_LIBRARY})
        list(APPEND plugin_SOURCES notifications_growl.mm)
    endif()

    list(APPEND plugin_LIBRARIES "-framework Carbon")
    list(APPEND plugin_SOURCES idle/idle_mac.cpp)
elseif (UNIX)
    # libnotify support
    pkg_check_modules(LIBNOTIFY "libnotify")
    if(LIBNOTIFY_FOUND)
        message(STATUS "Found libnotify library")
        include_directories(${LIBNOTIFY_INCLUDE_DIRS})
        add_definitions(-DUSE_LIBNOTIFY)
        list(APPEND plugin_LIBRARIES ${LIBNOTIFY_LIBRARIES})
        list(APPEND plugin_SOURCES notifications_libnotify.cpp)
    endif()

    # idle
    find_library(X11_LIBRARY NAMES X11)
    find_library(XSS_LIBRARY NAMES Xss)
    check_include_files(X11/extensions/scrnsaver.h HAVE_SCRNSAVER_H)
    if(X11_LIBRARY AND XSS_LIBRARY AND HAVE_SCRNSAVER_H)
        message(STATUS "Found X11 Screen saver library: ${XSS_LIBRARY} ${X11_LIBRARY}")
        list(APPEND plugin_LIBRARIES Qt5::X11Extras ${XSS_LIBRARY} ${X11_LIBRARY})
        list(APPEND plugin_SOURCES idle/idle_x11.cpp)
    else()
        list(APPEND plugin_SOURCES idle/idle_stub.cpp)
    endif()
elseif(WIN32)
    list(APPEND plugin_SOURCES idle/idle_win.cpp)
endif()

qt5_add_resources(plugin_RCC_SOURCES ../../data/wiLink.qrc)
add_library(qmlwilinkplugin SHARED ${plugin_SOURCES} ${plugin_RCC_SOURCES})
target_link_libraries(qmlwilinkplugin qnetio qxmpp-extra qsound ${QXMPP_LIBRARIES} ${plugin_LIBRARIES}
    Qt5::Gui Qt5::Network Qt5::Quick Qt5::Widgets Qt5::Xml)
if(WIN32)
    set_target_properties(qmlwilinkplugin PROPERTIES PREFIX "")
endif()

install(FILES qmldir DESTINATION ${WILINK_IMPORTS_DIR}/wiLink)
install(TARGETS qmlwilinkplugin DESTINATION ${WILINK_IMPORTS_DIR}/wiLink)
