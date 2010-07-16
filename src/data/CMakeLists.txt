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
    contact-away.svg
    contact-busy.svg
    contact-offline.svg
    diagnostics.svg
    download.svg
    file.svg
    forward.svg
    options.svg
    peer.svg
    photos.svg
    refresh.svg
    remove.svg
    search.svg
    start.svg
    stop.svg
    upload.svg
    wiLink.svg
    wiLink-black.svg)
generate_xpm(xpm wiLink.svg)

# large icons
add_custom_command(OUTPUT album-128.png DEPENDS album.svg
    COMMAND ${INKSCAPE} -z --file=${CMAKE_CURRENT_SOURCE_DIR}/album.svg --export-png=album-128.png --export-width=128 --export-height=128)
add_custom_command(OUTPUT file-128.png DEPENDS file.svg
    COMMAND ${INKSCAPE} -z --file=${CMAKE_CURRENT_SOURCE_DIR}/file.svg --export-png=file-128.png --export-width=128 --export-height=128)
add_custom_command(OUTPUT peer-128.png DEPENDS peer.svg
    COMMAND ${INKSCAPE} -z --file=${CMAKE_CURRENT_SOURCE_DIR}/peer.svg --export-png=peer-128.png --export-width=128 --export-height=128)
list(APPEND png album-128.png file-128.png peer-128.png)

add_custom_target(icons DEPENDS ${icns} ${ico} ${png} ${xpm})

