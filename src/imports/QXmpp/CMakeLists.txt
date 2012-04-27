set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

qt4_wrap_cpp(MOC_SOURCES plugin.h)
add_library(qmlqxmppplugin SHARED plugin.cpp ${MOC_SOURCES})
target_link_libraries(qmlqxmppplugin ${QXMPP_LIBRARIES} ${QT_LIBRARIES})

install(FILES qmldir DESTINATION bin/imports/QXmpp)
install(TARGETS qmlqxmppplugin DESTINATION bin/imports/QXmpp)
