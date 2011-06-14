include(IconUtilities)
generate_icns(icns wiLink.svg)
generate_ico(ico wiLink.svg)
generate_xpm(xpm wiLink.svg)

# 16x16 icons
generate_png(png SIZE 16
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
    chat.svg
    close.svg
    diagnostics.svg
    download.svg
    favorite-active.svg
    favorite-inactive.svg
    file.svg
    forward.svg
    hangup.svg
    home.svg
    invite.svg
    options.svg
    peer.svg
    permissions.svg
    phone.svg
    photos.svg
    plugin.svg
    refresh.svg
    remove.svg
    resize.svg
    search.svg
    share.svg
    start.svg
    stop.svg
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

