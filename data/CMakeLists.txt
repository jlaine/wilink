include(IconUtilities)
generate_icns(icns wDesktop.svg)
generate_ico(ico wDesktop.svg)
generate_png(png
    add.svg
    contact-available.svg
    contact-busy.svg
    contact-offline.svg
    remove.svg
    wDesktop.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${png})

qt4_add_translation(QM_FILES wDesktop.fr.ts)
add_custom_target(translations DEPENDS ${QM_FILES})
