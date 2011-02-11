set(QT_USE_QTMULTIMEDIA 1)
include(${QT_USE_FILE})

set(qsound_LIBRARIES)

# check macports on OS X
if(APPLE)
  set(CMAKE_REQUIRED_INCLUDES /opt/local/include)
  include_directories(/opt/local/include)
endif(APPLE)

# mad support
find_library(MAD_LIBRARY mad)
if(MAD_LIBRARY)
  check_include_files(mad.h HAVE_MAD_H)
  if(HAVE_MAD_H)
    message(STATUS "Found MAD library")
    add_definitions(-DUSE_MAD)
    set(qsound_LIBRARIES ${qsound_LIBRARIES} ${MAD_LIBRARY})
  endif()
endif()

# vorbisfile support
find_library(VORBISFILE_LIBRARY vorbisfile)
if(VORBISFILE_LIBRARY)
  check_include_files(vorbis/vorbisfile.h HAVE_VORBISFILE_H)
  if(HAVE_VORBISFILE_H)
    message(STATUS "Found vorbisfile library")
    add_definitions(-DUSE_VORBISFILE)
    set(qsound_LIBRARIES ${qsound_LIBRARIES} ${VORBISFILE_LIBRARY})
  endif()
endif()

SET(qsound_SOURCES QSoundFile.cpp QSoundMeter.cpp QSoundPlayer.cpp)
qt4_wrap_cpp(qsound_MOC_SOURCES QSoundFile.h QSoundMeter.h QSoundPlayer.h)
add_library(qsound STATIC ${qsound_SOURCES} ${qsound_MOC_SOURCES})
target_link_libraries(qsound ${QT_LIBRARIES} ${qsound_LIBRARIES})
