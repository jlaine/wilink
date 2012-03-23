# Disable dependency on QtGui
set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../qdjango/src/db)

set(qxmpp-shares_SOURCES
    QXmppShareDatabase.cpp
    QXmppShareIq.cpp
    QXmppShareManager.cpp)

set(qxmpp-shares_MOC_HEADERS
    QXmppShareDatabase.h
    QXmppShareDatabase_p.h
    QXmppShareManager.h)

qt4_wrap_cpp(qxmpp-shares_MOC_SOURCES ${qxmpp-shares_MOC_HEADERS})
add_library(qxmpp-shares STATIC ${qxmpp-shares_SOURCES} ${qxmpp-shares_MOC_SOURCES})
target_link_libraries(qxmpp-shares ${QT_LIBRARIES})
