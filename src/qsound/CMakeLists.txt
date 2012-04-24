set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(qsound_LIBRARIES)

# needed for Qt >= 4.7 on Debian/Ubuntu
find_library(QTMULTIMEDIAKIT_LIBRARY QtMultimediaKit)
if(QTMULTIMEDIAKIT_LIBRARY)
    message(STATUS "Found QtMultimediaKit library")
    include_directories(/usr/include/QtMobility)
    include_directories(/usr/include/QtMultimediaKit)
    list(APPEND qsound_LIBRARIES ${QTMULTIMEDIAKIT_LIBRARY})
endif()

add_subdirectory(src)
if(NOT QSOUND_DONT_USE_TESTS)
    add_subdirectory(tests)
endif()
