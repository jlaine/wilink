include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Until binary stability is reached, build static libraries
if(NOT QNETIO_LIBRARY_TYPE)
  set(QNETIO_LIBRARY_TYPE STATIC)
endif()

set(ALL_LIBRARIES Qt5::Network Qt5::Xml)

# Needed for plugin support
add_definitions(-DQT_STATICPLUGIN)
if(UNIX)
  if(NOT APPLE)
    add_definitions(-fPIC)
  endif(NOT APPLE)
endif(UNIX)

set(qnetio_SOURCES wallet.cpp)

if(APPLE)
  # OS X Keychain support
  message(STATUS "Found OS X Security library support")
  find_library(APPLE_FOUNDATION_LIBRARY NAMES Foundation)
  find_library(APPLE_SECURITY_LIBRARY NAMES Security)
  add_definitions(-DUSE_OSX_KEYCHAIN)
  set(qnetio_SOURCES ${qnetio_SOURCES} wallet/osx.cpp)
  set(ALL_LIBRARIES ${ALL_LIBRARIES} ${APPLE_FOUNDATION_LIBRARY} ${APPLE_SECURITY_LIBRARY})
elseif(WIN32)
  # Windows keyring support
  message(STATUS "Found Windows keyring support")
  add_definitions(-DUSE_WINDOWS_KEYRING)
  set(qnetio_SOURCES ${qnetio_SOURCES} wallet/windows.cpp)
else()
  set(qnetio_SOURCES ${qnetio_SOURCES} wallet/dummy.cpp)
endif()

add_library(qnetio ${QNETIO_LIBRARY_TYPE} ${qnetio_SOURCES})
set_target_properties(qnetio PROPERTIES SOVERSION 0)
target_link_libraries(qnetio ${ALL_LIBRARIES})

