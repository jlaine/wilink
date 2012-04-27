if(UNIX)
  if(NOT APPLE)
    add_definitions(-fPIC)
  endif(NOT APPLE)
endif(UNIX)

# mad support
find_library(MAD_LIBRARY mad)
if(MAD_LIBRARY)
  check_include_files(mad.h HAVE_MAD_H)
  if(HAVE_MAD_H)
    message(STATUS "Found MAD library")
    add_definitions(-DQSOUND_USE_MAD)
    list(APPEND qsound_LIBRARIES ${MAD_LIBRARY})
  endif()
endif()

# vorbisfile support
find_library(VORBISFILE_LIBRARY vorbisfile)
if(VORBISFILE_LIBRARY)
  check_include_files(vorbis/vorbisfile.h HAVE_VORBISFILE_H)
  if(HAVE_VORBISFILE_H)
    message(STATUS "Found vorbisfile library")
    add_definitions(-DQSOUND_USE_VORBISFILE)
    list(APPEND qsound_LIBRARIES ${VORBISFILE_LIBRARY})
  endif()
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

SET(qsound_SOURCES QSoundFile.cpp QSoundMeter.cpp QSoundPlayer.cpp QSoundStream.cpp QSoundTester.cpp QVideoGrabber.cpp)
if(WIN32)
    list(APPEND qsound_SOURCES QVideoGrabber_win.cpp)
elseif(APPLE)
    list(APPEND qsound_SOURCES QVideoGrabber_mac.mm)
else()
    list(APPEND qsound_SOURCES QVideoGrabber_linux.cpp)
endif()

qt4_wrap_cpp(qsound_MOC_SOURCES QSoundFile.h QSoundMeter.h QSoundPlayer.h QSoundStream.h QSoundTester.h QVideoGrabber.h)
add_library(qsound STATIC ${qsound_SOURCES} ${qsound_MOC_SOURCES})
target_link_libraries(qsound ${QT_LIBRARIES} ${qsound_LIBRARIES})
