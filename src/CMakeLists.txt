add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qnetio)
add_subdirectory(qxmpp)

set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/qxmpp)

# wiLink

add_definitions(-DQT_STATICPLUGIN)
set(wiLink_SOURCES
    application.cpp
    chat.cpp
    chat_accounts.cpp
    chat_client.cpp
    chat_conversation.cpp
    chat_dialog.cpp
    chat_edit.cpp
    chat_form.cpp
    chat_history.cpp
    chat_panel.cpp
    chat_roster.cpp
    chat_roster_item.cpp
    chat_tip.cpp
    main.cpp
    systeminfo.cpp
    updates.cpp
    updatesdialog.cpp
    utils.cpp
    plugins/console.cpp
    plugins/contacts.cpp
    plugins/diagnostics.cpp
    plugins/diagnostics/networkinfo.cpp
    plugins/menu.cpp
    plugins/photos.cpp
    plugins/rooms.cpp
    plugins/shares.cpp
    plugins/shares/database.cpp
    plugins/shares/model.cpp
    plugins/shares/view.cpp
    plugins/transfers.cpp)

set(wiLink_MOC_HEADERS
    application.h
    chat.h
    chat_accounts.h
    chat_client.h
    chat_conversation.h
    chat_dialog.h
    chat_edit.h
    chat_form.h
    chat_history.h
    chat_panel.h
    chat_plugin.h
    chat_roster.h
    chat_tip.h
    updates.h
    updatesdialog.h
    plugins/console.h
    plugins/contacts.h
    plugins/diagnostics.h
    plugins/menu.h
    plugins/photos.h
    plugins/rooms.h
    plugins/shares.h
    plugins/shares/database.h
    plugins/shares/model.h
    plugins/shares/view.h
    plugins/transfers.h)

configure_file(translations/translations.qrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
if(RELOAD_TRANSLATIONS)
    qt4_create_translation(QM_FILES ${wiLink_SOURCES} translations/fr.ts)
else(RELOAD_TRANSLATIONS)
    qt4_add_translation(QM_FILES translations/fr.ts)
endif(RELOAD_TRANSLATIONS)
qt4_add_translation(QM_FILES translations/qt_fr.ts)

set(wiLink_RCCS
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/wiLink.qrc)

if(WIN32)
    set(wiLink_SOURCES ${wiLink_SOURCES} plugins/diagnostics/wireless_win.cpp)
elseif(APPLE)
    set(wiLink_SOURCES ${wiLink_SOURCES} application_mac.mm plugins/diagnostics/wireless_mac.mm)
elseif(UNIX)
    set(wiLink_SOURCES ${wiLink_SOURCES} plugins/diagnostics/wireless_nm.cpp)
else(WIN32)
    set(wiLink_SOURCES ${wiLink_SOURCES} plugins/diagnostics/wireless_stub.cpp)
endif(WIN32)

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
if(APPLE)
    set(wilink_LIBRARIES "-framework AppKit")
endif(APPLE)
target_link_libraries(wiLink idle qnetio qxmpp ${wilink_LIBRARIES} ${QT_LIBRARIES} ${WLAN_LIBRARIES})

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
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins ${BUNDLE_LOCATION} ${WILINK_EXTRA_PLUGINS}
        DEPENDS wiLink)
endif(APPLE)
if(WIN32)
    set_target_properties(wiLink PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wiLink DESTINATION bin)

# tests
set(tests_MOC_HEADERS tests.h updates.h)
qt4_wrap_cpp(tests_MOC_SOURCES ${tests_MOC_HEADERS})
add_executable(tests systeminfo.cpp tests.cpp updates.cpp utils.cpp ${tests_MOC_SOURCES})
target_link_libraries(tests qnetio ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
