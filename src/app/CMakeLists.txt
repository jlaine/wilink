set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

# Libraries
set(app_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY} ${QT_QTNETWORK_LIBRARY})

# wiLink
add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")
if(WILINK_EMBEDDED)
    add_definitions(-DWILINK_EMBEDDED)
endif()

set(app_SOURCES application.cpp main.cpp window.cpp)
set(app_MOC_HEADERS application.h window.h)

if(APPLE)
    list(APPEND app_SOURCES application_mac.mm)
    list(APPEND app_LIBRARIES "-framework AppKit")
endif()

qt4_wrap_cpp(app_MOC_SOURCES ${app_MOC_HEADERS})
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

qt4_add_resources(RCC_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/wiLink.qrc)

if(WIN32)
    enable_language(RC)
    set(WILINK_VERSION_WIN32 "${WILINK_VERSION_MAJOR},${WILINK_VERSION_MINOR},${WILINK_VERSION_PATCH},0")
    configure_file(${CMAKE_SOURCE_DIR}/cmake/wiLink.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wiLink.rc)
    set(WILINK_RESOURCE wiLink.rc)
endif(WIN32)

add_executable(wiLink ${app_SOURCES} ${app_MOC_SOURCES} ${RCC_SOURCES} ${WILINK_RESOURCE})
target_link_libraries(wiLink ${app_LIBRARIES})

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
        COMMAND touch ${BUNDLE_LOCATION}/Contents/Resources/empty.lproj
        COMMAND cp ${CPACK_BUNDLE_ICON} ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins -b ${BUNDLE_LOCATION} -q ${QT_QMAKE_EXECUTABLE} -f "${WILINK_EXTRA_FRAMEWORKS}" -i "${WILINK_EXTRA_IMPORTS}" ${WILINK_EXTRA_PLUGINS}
        DEPENDS wiLink)
endif(APPLE)
if(WIN32)
    set_target_properties(wiLink PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wiLink DESTINATION bin)

# tests
#add_subdirectory(tests)
