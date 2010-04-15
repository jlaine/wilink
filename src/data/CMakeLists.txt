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
    options.svg
    photos.svg
    remove.svg
    updates.svg
    wiLink.svg
    wiLink-black.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${png})

