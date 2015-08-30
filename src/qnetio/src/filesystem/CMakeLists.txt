# needed for win32
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)

# GPhoto2 support
find_library(LIBGPHOTO_LIBRARY gphoto2)
if(LIBGPHOTO_LIBRARY)
  message(STATUS "Found GPhoto2 library: ${LIBGPHOTO_LIBRARY}")
  add_library(qnetio-gphoto SHARED gphoto.cpp)
  target_link_libraries(qnetio-gphoto qnetio ${LIBGPHOTO_LIBRARY})
  install(TARGETS qnetio-gphoto
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}/filesystem
    RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}/filesystem)
endif(LIBGPHOTO_LIBRARY)

