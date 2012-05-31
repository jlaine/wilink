set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
