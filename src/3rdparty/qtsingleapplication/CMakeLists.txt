include(${QT_USE_FILE})

qt4_wrap_cpp(MOC_SOURCES
    qtlocalpeer.h
    qtsingleapplication.h
    qtsinglecoreapplication.h)

set(SOURCES
    qtlocalpeer.cpp
    qtlockedfile.cpp
    qtsingleapplication.cpp
    qtsinglecoreapplication.cpp)
if(WIN32)
    list(APPEND SOURCES qtlockedfile_win.cpp)
else()
    list(APPEND SOURCES qtlockedfile_unix.cpp)
endif()

add_library(qtsingleapplication ${SOURCES} ${MOC_SOURCES})
target_link_libraries(qtsingleapplication ${QT_LIBRARIES})
