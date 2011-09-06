add_definitions(-DQXMPP_STREAM_LOG_PACKETS)
set(QDJANGO_DONT_USE_TESTS 1)
set(QNETIO_DONT_USE_TESTS 1)
set(QSOUND_DONT_USE_TESTS 1)
set(QXMPP_DONT_USE_TESTS 1)
set(QXMPP_DONT_USE_VPX 1)

add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qdjango)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp)
add_subdirectory(plugins)

# tests
add_subdirectory(tests)
