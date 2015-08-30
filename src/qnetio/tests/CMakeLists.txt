set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(qnetio-filesystem qnetio-filesystem.cpp)
target_link_libraries(qnetio-filesystem qnetio)

add_executable(qnetio-wallet qnetio-wallet.cpp)
target_link_libraries(qnetio-wallet qnetio ${QT_QTTEST_LIBRARY})

#install(TARGETS qnetio-wallet DESTINATION bin)
