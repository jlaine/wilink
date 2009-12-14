include(IconUtilities)
generate_icns(icns wDesktop.svg)
generate_ico(ico wDesktop.svg)
generate_png(png
    add.svg
    chat.svg
    contact-available.svg
    contact-busy.svg
    contact-offline.svg
    diagnostics.svg
    photos.svg
    remove.svg
    wDesktop.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${png})

