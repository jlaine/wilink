# Common definitions

QSOUND_INCLUDE_DIR = $$PWD
QSOUND_LIBRARY_DIR = $$PWD
QSOUND_LIBRARY_NAME = qsound

android {
    QSOUND_INCLUDE_DIR += $$PWD/fake
} else {
    CONFIG += mobility
    MOBILITY += multimedia
}

# Libraries for apps which use QNetIO
symbian {
    # Symbian needs a .lib extension to recognise the library as static
    QSOUND_LIBS = -L$$QSOUND_LIBRARY_DIR -l$${QSOUND_LIBRARY_NAME}.lib
} else {
    QSOUND_LIBS = -L$$QSOUND_LIBRARY_DIR -l$${QSOUND_LIBRARY_NAME}
}

