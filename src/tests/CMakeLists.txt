include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qsound/src)
add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")

# tests
#qt5_add_resources(tests_RCC_SOURCES tones.qrc)
#add_executable(tests
#    ../plugins/declarative.cpp
#    ../plugins/systeminfo.cpp
#    ../plugins/updates.cpp
#    ../plugins/utils.cpp
#    tests.cpp
#    ${tests_RCC_SOURCES})
#target_link_libraries(tests qnetio qsound qxmpp diagnostics Qt5::Network Qt5::Xml Qt5::Test)

add_executable(codec codec.cpp)
target_link_libraries(codec ${QXMPP_LIBRARIES} Qt5::Network Qt5::Xml)

#add_executable(phone
#    phone.cpp
#    ../plugins/phone/sip.cpp
#    ../plugins/phone/stun.cpp)
#target_link_libraries(phone qsound qxmpp Qt5::Network Qt5::Xml)

add_executable(hasher hasher.cpp)
target_link_libraries(hasher ${QXMPP_LIBRARIES} Qt5::Network Qt5::Xml)
