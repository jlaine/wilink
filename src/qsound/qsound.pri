# Common definitions

CONFIG += mobility
MOBILITY += multimedia
QSOUND_INCLUDE_DIR = $$PWD
QSOUND_LIBRARY_DIR = $$PWD
QSOUND_LIBRARY_NAME = qsound

# Libraries for apps which use QNetIO
symbian {
    # Symbian needs a .lib extension to recognise the library as static
    QSOUND_LIBS = -L$$QSOUND_LIBRARY_DIR -l$${QSOUND_LIBRARY_NAME}.lib
} else {
    QSOUND_LIBS = -L$$QSOUND_LIBRARY_DIR -l$${QSOUND_LIBRARY_NAME}
}

