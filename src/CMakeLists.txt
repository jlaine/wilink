add_subdirectory(qnetio)

set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

set(boc-desktop_SOURCES
    main.cpp
    photos.cpp
    trayicon.cpp)

set(boc-desktop_MOC_HEADERS
    photos.h
    trayicon.h)

qt4_wrap_cpp(boc-desktop_MOC_SOURCES ${boc-desktop_MOC_HEADERS})

add_executable(boc-desktop ${boc-desktop_SOURCES} ${boc-desktop_MOC_SOURCES} ${BOC_RESOURCE})
target_link_libraries(boc-desktop qnetio ${QT_LIBRARIES})
if(WIN32)
	set_target_properties(boc-desktop PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

