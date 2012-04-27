set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qnetio/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qsound/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qxmpp-extra/diagnostics)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../qxmpp-extra/shares)

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

# Needed for Qt >= 4.7 on Debian/Ubuntu
find_library(QTMULTIMEDIAKIT_LIBRARY QtMultimediaKit)
if(QTMULTIMEDIAKIT_LIBRARY)
    message(STATUS "Found QtMultimediaKit library")
    include_directories(/usr/include/QtMobility)
    include_directories(/usr/include/QtMultimediaKit)
endif()

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
    diagnostics.cpp
    discovery.cpp
    history.cpp
    icons.cpp
    idle/idle.cpp
    model.cpp
    news.cpp
    notifications.cpp
    photos.cpp
    phone.cpp
    phone/sip.cpp
    phone/stun.cpp
    places.cpp
    player.cpp
    rooms.cpp
    roster.cpp
    settings.cpp
    shares.cpp
    systeminfo.cpp
    updater.cpp)

set(plugin_MOC_HEADERS
    accounts.h
    calls.h
    client.h
    console.h
    conversations.h
    declarative.h
    diagnostics.h
    discovery.h
    history.h
    idle/idle.h
    model.h
    news.h
    notifications.h
    photos.h
    phone.h
    phone/sip.h
    phone/stun.h
    places.h
    player.h
    rooms.h
    roster.h
    settings.h
    shares.h
    updater.h)

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
    find_package(PkgConfig)
    pkg_check_modules(LIBNOTIFY "libnotify")
    if(LIBNOTIFY_FOUND)
        message(STATUS "Found libnotify library")
        include_directories(${LIBNOTIFY_INCLUDE_DIRS})
        add_definitions(-DUSE_LIBNOTIFY)
        list(APPEND plugin_LIBRARIES ${LIBNOTIFY_LIBRARIES})
        list(APPEND plugin_SOURCES notifications_libnotify.cpp)
    endif()

    # idle
    list(APPEND plugin_SOURCE idle/idle_x11.cpp)
    find_library(X11_LIBRARY NAMES X11)
    find_library(XSS_LIBRARY NAMES Xss)
    check_include_files(X11/extensions/scrnsaver.h HAVE_SCRNSAVER_H)
    if(X11_LIBRARY AND XSS_LIBRARY AND HAVE_SCRNSAVER_H)
        message(STATUS "Found X11 Screen saver library: ${XSS_LIBRARY} ${X11_LIBRARY}")
        add_definitions(-DHAVE_XSS)
        list(APPEND plugin_LIBRARIES ${XSS_LIBRARY} ${X11_LIBRARY})
    endif()
elseif(WIN32)
    list(APPEND plugin_SOURCES idle/idle_win.cpp)
endif()

qt4_wrap_cpp(plugin_MOC_SOURCES ${plugin_MOC_HEADERS})
add_library(qmlwilinkplugin SHARED ${plugin_SOURCES} ${plugin_MOC_SOURCES})
target_link_libraries(qmlwilinkplugin qnetio ${QXMPP_LIBRARIES} qxmpp-extra ${QDJANGO_LIBRARIES} qsound ${plugin_LIBRARIES} ${QT_LIBRARIES})
if(WIN32)
    set_target_properties(qmlwilinkplugin PROPERTIES PREFIX "")
endif()

install(FILES qmldir DESTINATION bin/imports/wiLink)
install(TARGETS qmlwilinkplugin DESTINATION bin/imports/wiLink)
