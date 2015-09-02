include_directories(../3rdparty/qtsingleapplication)

# Libraries
set(app_LIBRARIES Qt5::Gui Qt5::Network Qt5::Quick Qt5::Widgets Qt5::Xml qtsingleapplication)

# wiLink
add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")
if(WILINK_EMBEDDED)
    add_definitions(-DWILINK_EMBEDDED)
endif()

set(app_SOURCES main.cpp network.cpp window.cpp)

if(APPLE)
    list(APPEND app_SOURCES application_mac.mm)
    list(APPEND app_LIBRARIES "-framework AppKit")
elseif(UNIX)
    list(APPEND app_LIBRARIES X11)
endif()

qt5_add_resources(RCC_SOURCES app.qrc ../data/qml.qrc)

if(WIN32)
    enable_language(RC)
    set(WILINK_VERSION_WIN32 "${WILINK_VERSION_MAJOR},${WILINK_VERSION_MINOR},${WILINK_VERSION_PATCH},0")
    configure_file(${CMAKE_SOURCE_DIR}/cmake/wiLink.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wiLink.rc)
    set(WILINK_RESOURCE wiLink.rc)
endif(WIN32)

add_executable(wiLink ${app_SOURCES} ${RCC_SOURCES} ${WILINK_RESOURCE})
target_link_libraries(wiLink ${app_LIBRARIES})

if(APPLE)
    get_filename_component(BUNDLE_ICON_NAME ${CPACK_BUNDLE_ICON} NAME_WE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER ${CPACK_BUNDLE_IDENTIFIER})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_ICON_FILE ${BUNDLE_ICON_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${CPACK_BUNDLE_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${WILINK_VERSION})
    add_custom_command(TARGET wiLink POST_BUILD
        COMMAND mkdir -p ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND touch ${BUNDLE_LOCATION}/Contents/Resources/empty.lproj
        COMMAND cp ${CPACK_BUNDLE_ICON} ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins -b ${BUNDLE_LOCATION} -q ${QT_QMAKE_EXECUTABLE} -f Growl -i QtWebKit ${WILINK_EXTRA_PLUGINS}
        DEPENDS wiLink)
    add_dependencies(wiLink qmlwilinkplugin)
endif(APPLE)
if(WIN32)
    set_target_properties(wiLink PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wiLink DESTINATION bin)

# tests
#add_subdirectory(tests)
