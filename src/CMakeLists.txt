add_subdirectory(data)
add_subdirectory(qnetio)
add_subdirectory(qxmpp)

set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# tests

set(tests_MOC_HEADERS tests.h updates.h)
qt4_wrap_cpp(tests_MOC_SOURCES ${tests_MOC_HEADERS})
add_executable(tests systeminfo.cpp tests.cpp updates.cpp ${tests_MOC_SOURCES})
target_link_libraries(tests qnetio ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})

# wDesktop

set(wDesktop_SOURCES
    chat.cpp
    diagnostics.cpp
    main.cpp
    photos.cpp
    systeminfo.cpp
    trayicon.cpp
    updates.cpp)

set(wDesktop_MOC_HEADERS
    chat.h
    diagnostics.h
    photos.h
    trayicon.h
    updates.h)

configure_file(translations/translations.qrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
qt4_create_translation(QM_FILES ${wDesktop_SOURCES} translations/fr.ts)

set(wDesktop_RCCS
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/wDesktop.qrc)

if(WIN32)
  set(wDesktop_SOURCES ${wDesktop_SOURCES} wireless_win.cpp)
elseif(APPLE)
  set(wDesktop_SOURCES ${wDesktop_SOURCES} wireless_mac.mm)
elseif(UNIX)
  set(wDesktop_SOURCES ${wDesktop_SOURCES} wireless_nm.cpp)
else(WIN32)
  set(wDesktop_SOURCES ${wDesktop_SOURCES} wireless_stub.cpp)
endif(WIN32)

qt4_add_resources(wDesktop_RCC_SOURCES ${wDesktop_RCCS})
qt4_wrap_cpp(wDesktop_MOC_SOURCES ${wDesktop_MOC_HEADERS})

if(WIN32)
	enable_language(RC)
	set(WDESKTOP_VERSION_WIN32 "${WDESKTOP_VERSION_MAJOR},${WDESKTOP_VERSION_MINOR},${WDESKTOP_VERSION_PATCH},0")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/wDesktop.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wDesktop.rc)
    set(WDESKTOP_RESOURCE wDesktop.rc)
endif(WIN32)

add_executable(wDesktop ${wDesktop_SOURCES} ${wDesktop_MOC_SOURCES} ${wDesktop_RCC_SOURCES} ${WDESKTOP_RESOURCE})
target_link_libraries(wDesktop qnetio qxmpp ${QT_LIBRARIES} ${WLAN_LIBRARIES})
if(APPLE)
    get_filename_component(BUNDLE_ICON_NAME ${CPACK_BUNDLE_ICON} NAME_WE)
    set(BUNDLE_DMG ${CPACK_BUNDLE_NAME}-${CPACK_PACKAGE_VERSION}-mac.dmg)
	set(BUNDLE_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/wDesktop.app)
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE TRUE)
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER ${CPACK_BUNDLE_IDENTIFIER})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_ICON_FILE ${BUNDLE_ICON_NAME})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${CPACK_BUNDLE_NAME})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${CPACK_PACKAGE_VERSION})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${CPACK_PACKAGE_VERSION})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${CPACK_PACKAGE_VERSION})
	add_custom_command(TARGET wDesktop POST_BUILD
		COMMAND mkdir -p ${BUNDLE_LOCATION}/Contents/Resources/
		COMMAND cp ${CPACK_BUNDLE_ICON} ${BUNDLE_LOCATION}/Contents/Resources/
		COMMAND ${QT_BINARY_DIR}/macdeployqt ${BUNDLE_LOCATION} -no-plugins
		COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins ${BUNDLE_LOCATION} ${WDESKTOP_EXTRA_PLUGINS}
		DEPENDS wDesktop)
    # image generation
    set(_dmg_temp ${BUNDLE_DMG}.temp)
    add_custom_command(OUTPUT ${BUNDLE_DMG}
        COMMAND mkdir ${_dmg_temp}
        COMMAND cp -r ${BUNDLE_LOCATION} ${_dmg_temp}
        COMMAND ln -s /Applications ${_dmg_temp}/Applications
        COMMAND hdiutil create ${BUNDLE_DMG} -srcdir ${_dmg_temp} -fs HFS+ -volname "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION}"
        COMMAND rm -rf ${_dmg_temp}
        DEPENDS wDesktop)
    add_custom_target(dmg DEPENDS ${BUNDLE_DMG})
endif(APPLE)
if(WIN32)
    set_target_properties(wDesktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wDesktop DESTINATION bin)
