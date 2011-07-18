# fftw support
find_library(FFTW_LIBRARY fftw3)
if(FFTW_LIBRARY)
    qt4_add_resources(qsound-echo_RCC_SOURCES echo.qrc)
    qt4_wrap_cpp(qsound-echo_MOC_SOURCES echo.h)
    add_executable(qsound-echo echo.cpp ${qsound-echo_MOC_SOURCES} ${qsound-echo_RCC_SOURCES})
    target_link_libraries(qsound-echo qsound ${QT_LIBRARIES} ${FFTW_LIBRARY})
endif()

qt4_wrap_cpp(qvideo-grabber_MOC_SOURCES grabber.h)
add_executable(qvideo-grabber grabber.cpp ${qvideo-grabber_MOC_SOURCES})
target_link_libraries(qvideo-grabber qsound qxmpp ${QT_LIBRARIES})

qt4_add_resources(qsound-tones_RCC_SOURCES tones.qrc)
qt4_wrap_cpp(qsound-tones_MOC_SOURCES tones.h)
add_executable(qsound-tones tones.cpp ${qsound-tones_MOC_SOURCES} ${qsound-tones_RCC_SOURCES})
target_link_libraries(qsound-tones qsound ${QT_LIBRARIES})

