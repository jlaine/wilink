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

add_executable(wDesktop ${wDesktop_SOURCES} ${wDesktop_MOC_SOURCES} ${BOC_RESOURCE})
target_link_libraries(wDesktop qnetio ${QT_LIBRARIES})
if(WIN32)
	set_target_properties(wDesktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

