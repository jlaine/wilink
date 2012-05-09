set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
include(${QT_USE_FILE})

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

qt4_wrap_cpp(MOC_SOURCES plugin.h)
add_library(qmlqxmppplugin SHARED plugin.cpp ${MOC_SOURCES})
target_link_libraries(qmlqxmppplugin ${QXMPP_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTCORE_LIBRARY})
if(WIN32)
    set_target_properties(qmlqxmppplugin PROPERTIES PREFIX "")
endif()

install(FILES qmldir DESTINATION ${WILINK_IMPORTS_DIR}/QXmpp)
install(TARGETS qmlqxmppplugin DESTINATION ${WILINK_IMPORTS_DIR}/QXmpp)
