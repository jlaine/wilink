include(IconUtilities)
generate_icns(icns scalable/wiLink.svg)
generate_ico(ico scalable/wiLink.svg)
generate_xpm(xpm scalable/wiLink.svg)

# 10x10 images
generate_png(png SIZE 11
    scalable/down-arrow.svg
    scalable/eject.svg
    scalable/up-arrow.svg)

generate_png(png WIDTH 96 HEIGHT 32 scalable/tab.svg scalable/tab-active.svg)

set(SMALL_ICONS
    scalable/checkbox.svg
    scalable/checkbox-checked.svg)

set(ICONS
    scalable/add.svg
    scalable/album.svg
    scalable/audio-input.svg
    scalable/audio-output.svg
    scalable/back.svg
    scalable/call.svg
    scalable/call-incoming.svg
    scalable/call-outgoing.svg
    scalable/camera.svg
    scalable/camera-active.svg
    scalable/chat.svg
    scalable/clear.svg
    scalable/close.svg
    scalable/copy.svg
    scalable/cut.svg
    scalable/debug.svg
    scalable/diagnostics.svg
    scalable/dock-chat.svg
    scalable/dock-close.svg
    scalable/dock-diagnostics.svg
    scalable/dock-debug.svg
    scalable/dock-options.svg
    scalable/dock-peer.svg
    scalable/dock-phone.svg
    scalable/dock-photo.svg
    scalable/dock-rss.svg
    scalable/dock-share.svg
    scalable/dock-start.svg
    scalable/dock-web.svg
    scalable/download.svg
    scalable/favorite-active.svg
    scalable/favorite-inactive.svg
    scalable/file.svg
    scalable/fire.svg
    scalable/forward.svg
    scalable/fullscreen.svg
    scalable/hangup.svg
    scalable/home.svg
    scalable/information.svg
    scalable/invite.svg
    scalable/moderator.svg
    scalable/options.svg
    scalable/paste.svg
    scalable/peer.svg
    scalable/permissions.svg
    scalable/phone.svg
    scalable/photos.svg
    scalable/plugin.svg
    scalable/refresh.svg
    scalable/remove.svg
    scalable/resize.svg
    scalable/rss.svg
    scalable/search.svg
    scalable/share.svg
    scalable/start.svg
    scalable/stop.svg
    scalable/tip.svg
    scalable/transfers.svg
    scalable/upload.svg
    scalable/web.svg
    scalable/wiLink.svg
    scalable/wiLink-black.svg)

set(LARGE_ICONS
    scalable/album.svg
    scalable/file.svg
    scalable/peer.svg
    scalable/wiLink.svg)

# 16x16 icons
generate_png(png SIZE 16 DESTINATION 16x16 ${SMALL_ICONS})

# 32x32 icons
generate_png(png SIZE 32 DESTINATION 32x32 ${ICONS} ${SMALL_ICONS})

# 64x64 icons
generate_png(png SIZE 64 DESTINATION 64x64 ${ICONS})

# 128x128 icons
generate_png(png SIZE 128 DESTINATION 128x128 ${LARGE_ICONS})

add_custom_target(icons DEPENDS ${icns} ${ico} ${png} ${xpm})

if (UNIX AND NOT APPLE)
install(FILES wiLink.desktop DESTINATION share/applications)
install(FILES 32x32/wiLink.png DESTINATION share/icons/hicolor/32x32/apps)
install(FILES 64x64/wiLink.png DESTINATION share/icons/hicolor/64x64/apps)
install(FILES scalable/wiLink.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES wiLink.xpm DESTINATION share/pixmaps)
endif()
