# Common definitions

IDLE_INCLUDE_DIR = $$PWD
IDLE_LIBRARY_DIR = $$PWD

IDLE_LIBRARY_NAME = idle

# Libraries used internal by idle
symbian {

} else:mac {
    IDLE_INTERNAL_LIBS = -framework Carbon
} else:unix {
    IDLE_INTERNAL_LIBS = -lXss
}

# Libraries for apps which use idle
symbian {
    # Symbian needs a .lib extension to recognise the library as static
    IDLE_LIBS = -L$$IDLE_LIBRARY_DIR -l$${IDLE_LIBRARY_NAME}.lib
} else {
    IDLE_LIBS = -L$$IDLE_LIBRARY_DIR -l$${IDLE_LIBRARY_NAME}
}

IDLE_LIBS += $$IDLE_INTERNAL_LIBS
