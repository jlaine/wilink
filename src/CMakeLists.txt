set(QNETIO_DONT_USE_TESTS 1)
set(QNETIO_LIBRARY_TYPE STATIC)

add_definitions(${QXMPP_CFLAGS_OTHER})
include_directories(${QXMPP_INCLUDE_DIRS})
link_directories(${QDXMPP_LIBRARY_DIRS})

add_subdirectory(3rdparty)
add_subdirectory(data)
add_subdirectory(qnetio)
add_subdirectory(qsound)
add_subdirectory(qxmpp-extra)

add_subdirectory(imports)
add_subdirectory(app)
