include(IconUtilities)
generate_icns(icns wDesktop.svg)
generate_ico(ico wDesktop.svg)
generate_png(png
    add.svg
    back.svg
    chat.svg
    contact-available.svg
    contact-busy.svg
    contact-offline.svg
    diagnostics.svg
    options.svg
    photos.svg
    remove.svg
    updates.svg
    wDesktop.svg
    wDesktop-black.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${png})

