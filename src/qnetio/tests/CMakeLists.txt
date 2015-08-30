include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(qnetio-filesystem qnetio-filesystem.cpp)
target_link_libraries(qnetio-filesystem qnetio)

add_executable(qnetio-wallet qnetio-wallet.cpp)
target_link_libraries(qnetio-wallet qnetio Qt4::QtTest)

#install(TARGETS qnetio-wallet DESTINATION bin)
