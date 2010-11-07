set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

# tests
set(tests_MOC_HEADERS ../updates.h tests.h)
qt4_wrap_cpp(tests_MOC_SOURCES ${tests_MOC_HEADERS})
add_executable(tests
    ../chat_utils.cpp
    ../systeminfo.cpp
    ../updates.cpp
    tests.cpp
    ${tests_MOC_SOURCES})
target_link_libraries(tests qnetio qxmpp diagnostics ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})

add_executable(codec codec.cpp)
target_link_libraries(codec qxmpp ${QT_LIBRARIES})

add_executable(stun stun.cpp)
target_link_libraries(stun qxmpp ${QT_LIBRARIES})

add_executable(hasher hasher.cpp)
target_link_libraries(hasher qxmpp ${QT_LIBRARIES})
