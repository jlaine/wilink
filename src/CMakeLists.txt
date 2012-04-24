set(QNETIO_DONT_USE_TESTS 1)
set(QNETIO_LIBRARY_TYPE STATIC)
set(QSOUND_DONT_USE_TESTS 1)

# FIXME: remove hard-coded paths
include_directories(/usr/include/qdjango/db)
include_directories(/usr/include/qdjango/http)
include_directories(/usr/include/qdjango/script)
include_directories(${QXMPP_INCLUDE_DIRS})

add_subdirectory(data)
add_subdirectory(idle)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp-extra)

add_subdirectory(app)
add_subdirectory(tests)
