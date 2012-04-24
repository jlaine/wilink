add_definitions(-DQXMPP_STREAM_LOG_PACKETS)
set(QNETIO_DONT_USE_TESTS 1)
set(QNETIO_LIBRARY_TYPE STATIC)
set(QSOUND_DONT_USE_TESTS 1)
set(QXMPP_DONT_INSTALL 1)
set(QXMPP_DONT_USE_TESTS 1)
set(QXMPP_DONT_USE_VPX 1)
set(QXMPP_LIBRARY_TYPE STATIC)

# FIXME: remove hard-coded paths
include_directories(/usr/include/qdjango/db)
include_directories(/usr/include/qdjango/http)
include_directories(/usr/include/qdjango/script)
include_directories(/usr/include/qxmpp)

add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp-extra)

add_subdirectory(app)
add_subdirectory(tests)
