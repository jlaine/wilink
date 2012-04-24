set(QT_USE_QTNETWORK 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qsound/src)
add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")

# tests
#set(tests_MOC_HEADERS
#    ../plugins/declarative.h
#    ../plugins/updates.h
#    tests.h)
#qt4_wrap_cpp(tests_MOC_SOURCES ${tests_MOC_HEADERS})
#qt4_add_resources(tests_RCC_SOURCES tones.qrc)
#add_executable(tests
#    ../plugins/declarative.cpp
#    ../plugins/systeminfo.cpp
#    ../plugins/updates.cpp
#    ../plugins/utils.cpp
#    tests.cpp
#    ${tests_MOC_SOURCES}
#    ${tests_RCC_SOURCES})
#target_link_libraries(tests qnetio qsound qxmpp diagnostics ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})

add_executable(codec codec.cpp)
target_link_libraries(codec ${QXMPP_LIBRARIES} ${QT_LIBRARIES})

#qt4_wrap_cpp(phone_MOC_SOURCES
#    phone.h
#    ../plugins/phone/sip.h
#    ../plugins/phone/stun.h)
#add_executable(phone
#    phone.cpp
#    ../plugins/phone/sip.cpp
#    ../plugins/phone/stun.cpp
#    ${phone_MOC_SOURCES})
#target_link_libraries(phone qsound qxmpp ${QT_LIBRARIES})

qt4_wrap_cpp(stun_MOC_SOURCES stun.h)
add_executable(stun stun.cpp ${stun_MOC_SOURCES})
target_link_libraries(stun ${QXMPP_LIBRARIES} ${QT_LIBRARIES})

add_executable(hasher hasher.cpp)
target_link_libraries(hasher ${QXMPP_LIBRARIES} ${QT_LIBRARIES})
