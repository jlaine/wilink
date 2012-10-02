include(IconUtilities)
generate_icns(icns images/scalable/wiLink.svg)
generate_ico(ico images/scalable/wiLink.svg)
generate_xpm(xpm images/scalable/wiLink.svg)

set(ICONS
    images/scalable/album.svg
    images/scalable/audio-input.svg
    images/scalable/audio-output.svg
    images/scalable/call-incoming.svg
    images/scalable/call-outgoing.svg
    images/scalable/chat.svg
    images/scalable/close.svg
    images/scalable/file.svg
    images/scalable/moderator.svg
    images/scalable/peer.svg
    images/scalable/tip.svg
    images/scalable/web.svg
    images/scalable/wiLink.svg
    images/scalable/wiLink-black.svg)

set(LARGE_ICONS
    images/scalable/album.svg
    images/scalable/audio-x-generic.svgz
    images/scalable/file.svg
    images/scalable/image-x-generic.svgz
    images/scalable/peer.svg
    images/scalable/spinner.svg
    images/scalable/text-x-generic.svgz
    images/scalable/video-x-generic.svgz
    images/scalable/wiLink.svg)

# 32x32 icons
generate_png(png SIZE 32 DESTINATION images/32x32 ${ICONS})

# 64x64 icons
generate_png(png SIZE 64 DESTINATION images/64x64 ${ICONS})

# 80x80 icons
generate_png(png SIZE 80 DESTINATION images/80x80 images/scalable/wiLink.svg)

# 128x128 icons
generate_png(png SIZE 128 DESTINATION images/128x128 ${LARGE_ICONS})

add_custom_target(icons DEPENDS ${icns} ${ico} ${png} ${xpm})
if (UNIX AND NOT APPLE)
install(FILES wiLink.desktop DESTINATION share/applications)
install(FILES images/32x32/wiLink.png DESTINATION share/icons/hicolor/32x32/apps)
install(FILES images/64x64/wiLink.png DESTINATION share/icons/hicolor/64x64/apps)
install(FILES images/scalable/wiLink.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES wiLink.xpm DESTINATION share/pixmaps)
endif()
