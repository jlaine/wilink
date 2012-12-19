set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

qt4_wrap_cpp(qnetio-filesystem_MOC_SOURCES qnetio-filesystem.h)
add_executable(qnetio-filesystem qnetio-filesystem.cpp ${qnetio-filesystem_MOC_SOURCES})
target_link_libraries(qnetio-filesystem qnetio)

qt4_wrap_cpp(qnetio-wallet_MOC_SOURCES qnetio-wallet.h)
add_executable(qnetio-wallet qnetio-wallet.cpp ${qnetio-wallet_MOC_SOURCES})
target_link_libraries(qnetio-wallet qnetio ${QT_QTTEST_LIBRARY})

#install(TARGETS qnetio-wallet DESTINATION bin)
