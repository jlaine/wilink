set(QNETIO_DONT_USE_TESTS 1)
set(QNETIO_LIBRARY_TYPE STATIC)
set(QSOUND_DONT_USE_TESTS 1)

include_directories(${QDJANGO_INCLUDE_DIRS})
include_directories(${QXMPP_INCLUDE_DIRS})
link_directories(${QDJANGO_LIBRARY_DIRS})
link_directories(${QDXMPP_LIBRARY_DIRS})

add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp-extra)

add_subdirectory(app)
add_subdirectory(tests)
