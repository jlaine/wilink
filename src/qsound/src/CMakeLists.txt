if(UNIX)
  if(NOT APPLE)
    add_definitions(-fPIC)
  endif(NOT APPLE)
endif(UNIX)

set(qsound_LIBRARIES Qt5::Multimedia)

# mad support
pkg_check_modules(MAD mad)
if(MAD_FOUND)
    add_definitions(-DQSOUND_USE_MAD)
    list(APPEND qsound_LIBRARIES ${MAD_LIBRARIES})
endif()

# vorbisfile support
pkg_check_modules(VORBISFILE vorbisfile)
if(VORBISFILE_FOUND)
    add_definitions(-DQSOUND_USE_VORBISFILE)
    list(APPEND qsound_LIBRARIES ${VORBISFILE_LIBRARIES})
endif()

# DirectShow support
if(WIN32)
    list(APPEND qsound_LIBRARIES -lole32 -loleaut32 -lwinmm -luuid)
endif()

# QTKit support
find_library(QTKIT_LIBRARY NAMES QTKit)
find_library(COCOA_LIBRARY NAMES Cocoa)
find_library(QUARTZCORE_LIBRARY NAMES QuartzCore)
if(QTKIT_LIBRARY)
    message(STATUS "Found QTKit library: ${QTKIT_LIBRARY}")
    list(APPEND qsound_LIBRARIES ${QTKIT_LIBRARY} ${QUARTZCORE_LIBRARY} ${COCOA_LIBRARY})
endif()

SET(qsound_SOURCES QSoundFile.cpp QSoundLoader.cpp QSoundMeter.cpp QSoundPlayer.cpp QSoundStream.cpp QSoundTester.cpp QVideoGrabber.cpp)
if(WIN32)
    list(APPEND qsound_SOURCES QVideoGrabber_win.cpp)
elseif(APPLE)
    list(APPEND qsound_SOURCES QVideoGrabber_mac.mm)
else()
    list(APPEND qsound_SOURCES QVideoGrabber_linux.cpp)
endif()

add_library(qsound STATIC ${qsound_SOURCES})
target_link_libraries(qsound Qt5::Network Qt5::Quick ${qsound_LIBRARIES})
