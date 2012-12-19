include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Until binary stability is reached, build static libraries
if(NOT QNETIO_LIBRARY_TYPE)
  set(QNETIO_LIBRARY_TYPE STATIC)
endif()

add_subdirectory(filesystem)
add_subdirectory(wallet)

set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})
set(ALL_LIBRARIES ${QT_LIBRARIES})

# Needed for plugin support
add_definitions(-DQT_STATICPLUGIN)
if(UNIX)
  if(NOT APPLE)
    add_definitions(-fPIC)
  endif(NOT APPLE)
endif(UNIX)

set(qnetio_SOURCES
    mime.cpp)

set(qnetio_SOURCES ${qnetio_SOURCES}
    filesystem.cpp
    filesystem/file.cpp
    filesystem/ftp.cpp
    filesystem/http.cpp
    filesystem/picasa.cpp
    filesystem/wifirst.cpp
    wallet.cpp
    wallet/dummy.cpp)

set(qnetio_MOC_HEADERS
    filesystem.h
    filesystem/file.h
    filesystem/ftp.h
    filesystem/http.h
    filesystem/picasa.h
    filesystem/wifirst.h
    wallet.h
    wallet/dummy.h)

if(WIN32)
  # Windows keyring support
  message(STATUS "Found Windows keyring support")
  add_definitions(-DUSE_WINDOWS_KEYRING)
  set(qnetio_SOURCES ${qnetio_SOURCES} wallet/windows.cpp)
  set(qnetio_MOC_HEADERS ${qnetio_MOC_HEADERS} wallet/windows.h)
else(WIN32)
  # OS X Keychain support
  find_library(APPLE_FOUNDATION_LIBRARY NAMES Foundation)
  find_library(APPLE_SECURITY_LIBRARY NAMES Security)
  if(APPLE_FOUNDATION_LIBRARY AND APPLE_SECURITY_LIBRARY)
    message(STATUS "Found OS X Security library support : ${APPLE_SECURITY_LIBRARY}")
    add_definitions(-DUSE_OSX_KEYCHAIN)
    set(qnetio_SOURCES ${qnetio_SOURCES} wallet/osx.cpp)
    set(qnetio_MOC_HEADERS ${qnetio_MOC_HEADERS} wallet/osx.h)
    set(ALL_LIBRARIES ${ALL_LIBRARIES} ${APPLE_FOUNDATION_LIBRARY} ${APPLE_SECURITY_LIBRARY})
  endif(APPLE_FOUNDATION_LIBRARY AND APPLE_SECURITY_LIBRARY)
endif(WIN32)

qt4_wrap_cpp(qnetio_MOC_SOURCES ${qnetio_MOC_HEADERS})
add_library(qnetio ${QNETIO_LIBRARY_TYPE} ${qnetio_SOURCES} ${qnetio_MOC_SOURCES})
set_target_properties(qnetio PROPERTIES SOVERSION 0)
target_link_libraries(qnetio ${ALL_LIBRARIES})

