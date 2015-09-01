set(SOURCES
    qtlocalpeer.cpp
    qtlockedfile.cpp)
if(WIN32)
    list(APPEND SOURCES qtlockedfile_win.cpp)
else()
    list(APPEND SOURCES qtlockedfile_unix.cpp)
endif()

add_library(qtsingleapplication ${SOURCES})
target_link_libraries(qtsingleapplication Qt5::Core)
