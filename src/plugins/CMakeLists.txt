set(QT_USE_QTDECLARATIVE 1)
include(${QT_USE_FILE})

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

qt4_wrap_cpp(MOC_SOURCES folders.h)
add_library(qmlwilinkplugin SHARED folders.cpp ${MOC_SOURCES})
target_link_libraries(qmlwilinkplugin ${QT_LIBRARIES})
