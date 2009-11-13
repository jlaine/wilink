add_subdirectory(qnetio)

set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

set(wDesktop_SOURCES
    main.cpp
    photos.cpp
    trayicon.cpp)

set(wDesktop_MOC_HEADERS
    photos.h
    trayicon.h)

qt4_wrap_cpp(wDesktop_MOC_SOURCES ${wDesktop_MOC_HEADERS})

if(WIN32)
	enable_language(RC)
	set(WDESKTOP_VERSION_WIN32 "${WDESKTOP_VERSION_MAJOR},${WDESKTOP_VERSION_MINOR},${WDESKTOP_VERSION_PATCH},0")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/wDesktop.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wDesktop.rc)
    set(WDESKTOP_RESOURCE wDesktop.rc)
endif(WIN32)

add_executable(wDesktop ${wDesktop_SOURCES} ${wDesktop_MOC_SOURCES} ${WDESKTOP_RESOURCE})
target_link_libraries(wDesktop qnetio ${QT_LIBRARIES})
if(APPLE)
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE TRUE)
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.wifirst.wDesktop")
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_ICON_FILE "wDesktop.icns")
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "wDesktop")
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${CPACK_PACKAGE_VERSION})
	set_target_properties(wDesktop PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${CPACK_PACKAGE_VERSION})
	add_custom_command(TARGET wDesktop POST_BUILD
		COMMAND ${QT_BINARY_DIR}/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/wDesktop.app
		COMMAND cp ${CMAKE_SOURCE_DIR}/data/wDesktop.icns ${CMAKE_CURRENT_BINARY_DIR}/wDesktop.app/Contents/Resources
		DEPENDS wDesktop)
endif(APPLE)
if(WIN32)
	set_target_properties(wDesktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wDesktop DESTINATION bin)
