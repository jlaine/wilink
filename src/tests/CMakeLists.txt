set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")

# tests
set(tests_MOC_HEADERS
    ../plugins/updates.h
    tests.h)
qt4_wrap_cpp(tests_MOC_SOURCES ${tests_MOC_HEADERS})
qt4_add_resources(tests_RCC_SOURCES tones.qrc)
add_executable(tests
    ../plugins/systeminfo.cpp
    ../plugins/updates.cpp
    ../plugins/utils.cpp
    tests.cpp
    ${tests_MOC_SOURCES}
    ${tests_RCC_SOURCES})
target_link_libraries(tests qnetio qsound qxmpp diagnostics ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})

add_executable(codec codec.cpp)
target_link_libraries(codec qxmpp ${QT_LIBRARIES})

qt4_wrap_cpp(phone_MOC_SOURCES
    phone.h
    ../plugins/phone/sip.h
    ../plugins/phone/stun.h)
add_executable(phone
    phone.cpp
    ../plugins/phone/sip.cpp
    ../plugins/phone/stun.cpp
    ${phone_MOC_SOURCES})
target_link_libraries(phone qsound qxmpp ${QT_LIBRARIES})

qt4_wrap_cpp(stun_MOC_SOURCES stun.h)
add_executable(stun stun.cpp ${stun_MOC_SOURCES})
target_link_libraries(stun qxmpp ${QT_LIBRARIES})

qt4_add_resources(tones_RCC_SOURCES tones.qrc)
qt4_wrap_cpp(tones_MOC_SOURCES tones.h)
add_executable(tones tones.cpp ${tones_MOC_SOURCES} ${tones_RCC_SOURCES})
target_link_libraries(tones qsound ${QT_LIBRARIES})

qt4_wrap_cpp(grabber_MOC_SOURCES grabber.h)
add_executable(grabber grabber.cpp ${grabber_MOC_SOURCES})
target_link_libraries(grabber qsound qxmpp ${QT_LIBRARIES})

add_executable(hasher hasher.cpp)
target_link_libraries(hasher qxmpp ${QT_LIBRARIES})
