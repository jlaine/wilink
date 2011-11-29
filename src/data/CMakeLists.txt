include(IconUtilities)
generate_icns(icns wiLink.svg)
generate_ico(ico wiLink.svg)
generate_xpm(xpm wiLink.svg)

# 10x10 images
generate_png(png SIZE 11
    down-arrow.svg
    eject.svg
    up-arrow.svg)

# 16x16 icons
generate_png(png SIZE 16
    checkbox.svg
    checkbox-checked.svg
    smiley-angry.svg
    smiley-confused.svg
    smiley-happy.svg
    smiley-neutral.svg
    smiley-raspberry.svg
    smiley-sad.svg
    smiley-wink.svg)

# 32x32 icons
generate_png(png
    add.svg
    album.svg
    audio-input.svg
    audio-output.svg
    back.svg
    call.svg
    call-incoming.svg
    call-outgoing.svg
    camera.svg
    camera-active.svg
    chat.svg
    clear.svg
    close.svg
    copy.svg
    cut.svg
    debug.svg
    diagnostics.svg
    dock-chat.svg
    dock-diagnostics.svg
    dock-debug.svg
    dock-options.svg
    dock-peer.svg
    dock-phone.svg
    dock-photo.svg
    dock-rss.svg
    dock-share.svg
    dock-start.svg
    download.svg
    favorite-active.svg
    favorite-inactive.svg
    file.svg
    fire.svg
    forward.svg
    fullscreen.svg
    hangup.svg
    home.svg
    information.svg
    invite.svg
    moderator.svg
    options.svg
    paste.svg
    peer.svg
    permissions.svg
    phone.svg
    photos.svg
    plugin.svg
    refresh.svg
    remove.svg
    resize.svg
    rss.svg
    search.svg
    share.svg
    start.svg
    stop.svg
    tip.svg
    transfers.svg
    upload.svg
    wiLink.svg
    wiLink-black.svg)

# 64x64 icons
generate_png(png SIZE 64 SUFFIX -64
    wiLink.svg)

# 128x128 icons
generate_png(png SIZE 128 SUFFIX -128
    album.svg
    file.svg
    peer.svg)

add_custom_target(icons DEPENDS ${icns} ${ico} ${png} ${xpm})

