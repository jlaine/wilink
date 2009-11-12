add_subdirectory(qnetio)

set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

set(wdesktop_SOURCES
    main.cpp
    photos.cpp
    trayicon.cpp)

set(wdesktop_MOC_HEADERS
    photos.h
    trayicon.h)

qt4_wrap_cpp(wdesktop_MOC_SOURCES ${wdesktop_MOC_HEADERS})

add_executable(wdesktop ${wdesktop_SOURCES} ${wdesktop_MOC_SOURCES} ${BOC_RESOURCE})
target_link_libraries(wdesktop qnetio ${QT_LIBRARIES})
if(WIN32)
	set_target_properties(wdesktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

