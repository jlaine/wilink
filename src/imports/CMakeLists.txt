set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTWEBKIT 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qnetio/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qsound/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp-extra/diagnostics)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp-extra/shares)

# Libraries
set(app_LIBRARIES ${QT_QTGUI_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY} ${QT_QTWEBKIT_LIBRARY})

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

set(app_SOURCES application.cpp main.cpp window.cpp)
set(app_MOC_HEADERS application.h window.h)
qt4_wrap_cpp(app_MOC_SOURCES ${app_MOC_HEADERS})

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

if(APPLE)
    list(APPEND app_SOURCES application_mac.mm)
    list(APPEND app_LIBRARIES "-framework AppKit")

    # Growl support
    find_library(GROWL_LIBRARY Growl)
    if(GROWL_LIBRARY)
        message(STATUS "Found Growl library")
        add_definitions(-DUSE_GROWL)
        list(APPEND plugin_LIBRARIES ${GROWL_LIBRARY})
        list(APPEND plugin_SOURCES notifications_growl.mm)
    endif()
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
endif()

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

configure_file(../translations/translations.qrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
if(RELOAD_TRANSLATIONS)
    qt4_create_translation(QM_FILES
        application.cpp
        window.cpp
        ../data/AboutDialog.qml
        ../data/AccountAddPanel.qml
        ../data/AccountListPanel.qml
        ../data/AccountPasswordDialog.qml
        ../data/AccountPreferencePanel.qml
        ../data/CallNotification.qml
        ../data/CallWidget.qml
        ../data/ChatEdit.qml
        ../data/ChatPanel.qml
        ../data/ChatPlugin.qml
        ../data/ContactAddDialog.qml
        ../data/ContactAddNotification.qml
        ../data/ContactRenameDialog.qml
        ../data/ContactRemoveDialog.qml
        ../data/ConversationPanel.qml
        ../data/DebugPlugin.qml
        ../data/DiagnosticPanel.qml
        ../data/DiagnosticPlugin.qml
        ../data/Dialog.qml
        ../data/DiscoveryPanel.qml
        ../data/Dock.qml
        ../data/ErrorNotification.qml
        ../data/FolderQueueView.qml
        ../data/GeneralPreferencePanel.qml
        ../data/HistoryView.qml
        ../data/InputMenu.qml
        ../data/LogPanel.qml
        ../data/Main.qml
        ../data/NewsDialog.qml
        ../data/NewsPanel.qml
        ../data/NewsPlugin.qml
        ../data/PhoneContactDialog.qml
        ../data/PhoneContactView.qml
        ../data/PhoneHistoryView.qml
        ../data/PhoneNotification.qml
        ../data/PhonePanel.qml
        ../data/PhonePlugin.qml
        ../data/PhotoAlbumDialog.qml
        ../data/PhotoDeleteDialog.qml
        ../data/PhotoPanel.qml
        ../data/PhotoPlugin.qml
        ../data/PlayerPanel.qml
        ../data/PlayerPlugin.qml
        ../data/PluginPreferencePanel.qml
        ../data/PreferenceDialog.qml
        ../data/PreferenceModel.qml
        ../data/RoomInviteDialog.qml
        ../data/RoomInviteNotification.qml
        ../data/RoomJoinDialog.qml
        ../data/RoomKickDialog.qml
        ../data/RoomPanel.qml
        ../data/RoomParticipantView.qml
        ../data/RoomPermissionDialog.qml
        ../data/RoomSubjectDialog.qml
        ../data/SetupDialog.qml
        ../data/SharePanel.qml
        ../data/SharePlugin.qml
        ../data/SharePreferencePanel.qml
        ../data/ShareView.qml
        ../data/SoundPreferencePanel.qml
        ../data/StatusBar.qml
        ../data/TransferNotification.qml
        ../data/TransferWidget.qml
        ../data/WifirstPlugin.qml
        ../translations/fr.ts)
else(RELOAD_TRANSLATIONS)
    qt4_add_translation(QM_FILES ../translations/fr.ts)
endif(RELOAD_TRANSLATIONS)
qt4_add_translation(QM_FILES ../translations/qt_fr.ts)

qt4_wrap_cpp(plugin_MOC_SOURCES ${plugin_MOC_HEADERS})

add_library(qmlwilinkplugin STATIC ${plugin_SOURCES} ${plugin_MOC_SOURCES})
target_link_libraries(qmlwilinkplugin ${QT_LIBRARIES})
target_link_libraries(qmlwilinkplugin idle qnetio ${QXMPP_LIBRARIES} qxmpp-extra ${QDJANGO_LIBRARIES} qsound ${plugin_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY})

# tests
#add_subdirectory(tests)
