add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qdjango)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp)

set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/qdjango/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/qsound)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/qxmpp/src)

# Needed for Qt >= 4.7 on Debian/Ubuntu
find_library(QTMULTIMEDIAKIT_LIBRARY QtMultimediaKit)
if(QTMULTIMEDIAKIT_LIBRARY)
    message(STATUS "Found QtMultimediaKit library")
    include_directories(/usr/include/QtMobility)
    include_directories(/usr/include/QtMultimediaKit)
endif()

# For libnotify support
if (UNIX)
    find_package(PkgConfig)
    pkg_check_modules(LIBNOTIFY "libnotify")
    if(LIBNOTIFY_FOUND)
        message(STATUS "Found libnotify library")
        include_directories(${LIBNOTIFY_INCLUDE_DIRS})
        add_definitions(-DUSE_LIBNOTIFY)
    endif()
endif()

# wiLink

add_definitions(-DQT_STATICPLUGIN)
if(NOT APPLE)
    add_definitions(-DUSE_SYSTRAY)
endif(NOT APPLE)

set(wiLink_SOURCES
    application.cpp
    chat.cpp
    chat_accounts.cpp
    chat_client.cpp
    chat_form.cpp
    chat_history.cpp
    chat_model.cpp
    chat_preferences.cpp
    chat_search.cpp
    chat_status.cpp
    chat_utils.cpp
    main.cpp
    systeminfo.cpp
    updates.cpp
    updatesdialog.cpp
    plugins/calls.cpp
    plugins/console.cpp
    plugins/conversations.cpp
    plugins/declarative.cpp
    plugins/diagnostics.cpp
    plugins/discovery.cpp
    plugins/photos.cpp
    plugins/phone.cpp
    plugins/phone/sip.cpp
    plugins/phone/stun.cpp
    plugins/player.cpp
    plugins/rooms.cpp
    plugins/roster.cpp
    plugins/shares/model.cpp
    plugins/shares/options.cpp
    plugins/shares/view.cpp)

if(APPLE)
    set(wiLink_SOURCES ${wiLink_SOURCES} application_mac.mm)
endif(APPLE)

set(wiLink_MOC_HEADERS
    application.h
    chat.h
    chat_accounts.h
    chat_client.h
    chat_form.h
    chat_history.h
    chat_model.h
    chat_plugin.h
    chat_preferences.h
    chat_search.h
    chat_status.h
    updates.h
    updatesdialog.h
    plugins/calls.h
    plugins/console.h
    plugins/conversations.h
    plugins/declarative.h
    plugins/diagnostics.h
    plugins/discovery.h
    plugins/photos.h
    plugins/phone.h
    plugins/phone/sip.h
    plugins/phone/stun.h
    plugins/player.h
    plugins/rooms.h
    plugins/roster.h
    plugins/shares/options.h
    plugins/shares/model.h
    plugins/shares/view.h)

configure_file(translations/translations.qrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
if(RELOAD_TRANSLATIONS)
    qt4_create_translation(QM_FILES
        ${wiLink_SOURCES}
        data/main.qml
        data/CallWidget.qml
        data/ConversationPanel.qml
        data/DiagnosticPanel.qml
        data/Dialog.qml
        data/DiscoveryPanel.qml
        data/LogPanel.qml
        data/RoomInviteDialog.qml
        data/RoomJoinDialog.qml
        data/RoomParticipantView.qml
        data/PhoneHistoryView.qml
        data/PhonePanel.qml
        data/PhotoPanel.qml
        data/RoomPanel.qml
        data/SharePanel.qml
        data/TransferWidget.qml
        translations/fr.ts)
else(RELOAD_TRANSLATIONS)
    qt4_add_translation(QM_FILES translations/fr.ts)
endif(RELOAD_TRANSLATIONS)
qt4_add_translation(QM_FILES translations/qt_fr.ts)

set(wiLink_RCCS
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/wiLink.qrc)

qt4_add_resources(wiLink_RCC_SOURCES ${wiLink_RCCS})
qt4_wrap_cpp(wiLink_MOC_SOURCES ${wiLink_MOC_HEADERS})

if(WIN32)
    enable_language(RC)
    set(WILINK_VERSION_WIN32 "${WILINK_VERSION_MAJOR},${WILINK_VERSION_MINOR},${WILINK_VERSION_PATCH},0")
    configure_file(${CMAKE_SOURCE_DIR}/cmake/wiLink.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wiLink.rc)
    set(WILINK_RESOURCE wiLink.rc)
endif(WIN32)

add_executable(wiLink ${wiLink_SOURCES} ${wiLink_MOC_SOURCES} ${wiLink_RCC_SOURCES} ${WILINK_RESOURCE})

# libraries
set(wilink_LIBRARIES ${QT_QTGUI_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY})
if(APPLE)
    list(APPEND wilink_LIBRARIES "-framework AppKit")
endif(APPLE)

# For libnotify support
if(LIBNOTIFY_FOUND)
    list(APPEND wilink_LIBRARIES ${LIBNOTIFY_LIBRARIES})
endif()

target_link_libraries(wiLink idle qnetio qxmpp qdjango qsound diagnostics ${wilink_LIBRARIES})

if(APPLE)
    get_filename_component(BUNDLE_ICON_NAME ${CPACK_BUNDLE_ICON} NAME_WE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER ${CPACK_BUNDLE_IDENTIFIER})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_ICON_FILE ${BUNDLE_ICON_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${CPACK_BUNDLE_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${WILINK_VERSION})
    add_custom_command(TARGET wiLink POST_BUILD
        COMMAND mkdir -p ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND cp ${CPACK_BUNDLE_ICON} ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND ${QT_BINARY_DIR}/macdeployqt ${BUNDLE_LOCATION} -no-plugins
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins ${BUNDLE_LOCATION} ${QT_QMAKE_EXECUTABLE} ${WILINK_EXTRA_PLUGINS}
        DEPENDS wiLink)
endif(APPLE)
if(WIN32)
    set_target_properties(wiLink PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wiLink DESTINATION bin)

# tests
add_subdirectory(tests)
