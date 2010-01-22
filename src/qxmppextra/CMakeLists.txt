set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})
include_directories(${CMAKE_SOURCE_DIR}/src/qxmpp)

set(qxmppextra_SOURCES
    QXmppArchiveIq.cpp
    QXmppPingIq.cpp)

add_library(qxmppextra STATIC ${qxmppextra_SOURCES})
target_link_libraries(qxmppextra qxmpp)
