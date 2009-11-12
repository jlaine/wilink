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
if(WIN32)
	set_target_properties(wDesktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

ship_depends(wDesktop)
install(TARGETS wDesktop DESTINATION ${WDESKTOP_RUNTIME_DIR})
