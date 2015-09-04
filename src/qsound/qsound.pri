# Common definitions

QSOUND_INCLUDE_DIR = $$PWD/src
QSOUND_LIBRARY_NAME = qsound

QT += multimedia

# Libraries used internally by QSound
QSOUND_INTERNAL_DEFINES += QSOUND_USE_MAD
isEmpty(WILINK_SYSTEM_MAD) {
    QSOUND_INCLUDE_DIR += $$PWD/../3rdparty/libmad
}
QSOUND_INTERNAL_LIBS += -lmad

mac {
    # qtkit support
    QSOUND_INTERNAL_LIBS += -framework QTKit -framework QuartzCore -framework Cocoa
} else:win32 {
    # directshow support
    QSOUND_INTERNAL_LIBS += -lole32 -loleaut32 -lwinmm -luuid
}

# Libraries for apps which use QSound
QSOUND_LIBS = -l$${QSOUND_LIBRARY_NAME}

# FIXME: we should be able to use the link_prl option to automatically pull in
# the extra libraries which our library needs, but this does not seem to work.
QSOUND_LIBS += $$QSOUND_INTERNAL_LIBS
