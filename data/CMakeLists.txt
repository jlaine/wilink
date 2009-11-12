generate_icns(icns wDesktop.svg)
generate_ico(ico wDesktop.svg)
generate_xpm(xpm wDesktop.svg)
add_custom_target(icons DEPENDS ${icns} ${ico} ${xpm})
