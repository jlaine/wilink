TEMPLATE = lib
TARGET = mad
CONFIG += staticlib
DEFINES += FPM_DEFAULT
HEADERS = \
    bit.h \
    decoder.h \
    fixed.h \
    frame.h \
    global.h \
    huffman.h \
    layer12.h \
    layer3.h \
    mad.h \
    stream.h \
    synth.h \
    timer.h \
    version.h
SOURCES = \
    bit.c \
    decoder.c \
    fixed.c \
    frame.c \
    huffman.c \
    layer12.c \
    layer3.c \
    stream.c \
    synth.c \
    timer.c \
    version.c
