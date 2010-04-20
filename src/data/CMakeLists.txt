include(IconUtilities)
generate_icns(icns wiLink.svg)
generate_ico(ico wiLink.svg)
generate_png(png
    add.svg
    album.svg
    back.svg
    chat.svg
    close.svg
    contact-available.svg
    contact-busy.svg
    contact-offline.svg
    diagnostics.svg
    download.svg
    options.svg
    photos.svg
    refresh.svg
    remove.svg
    search.svg
    upload.svg
    wiLink.svg
    wiLink-black.svg)
generate_xpm(xpm wiLink.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${png} ${xpm})

