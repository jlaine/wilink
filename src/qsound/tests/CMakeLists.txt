include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# fftw support
qt4_add_resources(qsound-echo_RCC_SOURCES echo.qrc)
add_executable(qsound-echo echo.cpp ${qsound-echo_RCC_SOURCES})
target_link_libraries(qsound-echo qsound ${QT_LIBRARIES})

add_executable(qvideo-grabber grabber.cpp)
target_link_libraries(qvideo-grabber qsound qxmpp ${QT_LIBRARIES})

qt4_add_resources(qsound-tones_RCC_SOURCES tones.qrc)
add_executable(qsound-tones tones.cpp ${qsound-tones_RCC_SOURCES})
target_link_libraries(qsound-tones qsound ${QT_LIBRARIES})

