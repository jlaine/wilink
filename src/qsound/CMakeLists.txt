set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp/src)

set(qsound_LIBRARIES)

# check macports on OS X
if(APPLE)
  set(CMAKE_REQUIRED_INCLUDES /opt/local/include)
  include_directories(/opt/local/include)
endif(APPLE)

# needed for Qt >= 4.7 on Debian/Ubuntu
find_library(QTMULTIMEDIAKIT_LIBRARY QtMultimediaKit)
if(QTMULTIMEDIAKIT_LIBRARY)
    message(STATUS "Found QtMultimediaKit library")
    include_directories(/usr/include/QtMobility)
    include_directories(/usr/include/QtMultimediaKit)
    list(APPEND qsound_LIBRARIES ${QTMULTIMEDIAKIT_LIBRARY})
endif()

add_subdirectory(src)
add_subdirectory(tests)
