# Disable dependency on QtGui
set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTXML 1)
if(UNIX AND NOT APPLE AND NOT QXMPP_DONT_USE_QTDBUS)
    set(QT_USE_QTDBUS true)
endif()
include(${QT_USE_FILE})

# Wireless
find_library(COREWLAN_LIBRARY NAMES CoreWLAN)
if(COREWLAN_LIBRARY)
    message(STATUS "Found CoreWLAN library: ${COREWLAN_LIBRARY}")
    add_definitions(-DUSE_COREWLAN)
    set(WLAN_LIBRARIES ${COREWLAN_LIBRARY})
endif(COREWLAN_LIBRARY)
if(WIN32)
    set(wireless_SOURCES wireless_win.cpp)
elseif(APPLE)
    set(wireless_SOURCES wireless_mac.mm)
elseif(QT_USE_QTDBUS AND QT_QTDBUS_FOUND)
    set(wireless_SOURCES wireless_nm.cpp)
else(WIN32)
    set(wireless_SOURCES wireless_stub.cpp)
endif(WIN32)

set(qxmpp-diagnostics_SOURCES
    interface.cpp
    iq.cpp
    network.cpp
    software.cpp
    transfer.cpp
    wireless.cpp
    ${wireless_SOURCES})
set(qxmpp-diagnostics_MOC_HEADERS
    transfer.h)

qt4_wrap_cpp(qxmpp-diagnostics_MOC_SOURCES ${qxmpp-diagnostics_MOC_HEADERS})
add_library(qxmpp-diagnostics STATIC ${qxmpp-diagnostics_SOURCES} ${qxmpp-diagnostics_MOC_SOURCES})
target_link_libraries(qxmpp-diagnostics ${QT_LIBRARIES} ${WLAN_LIBRARIES})
