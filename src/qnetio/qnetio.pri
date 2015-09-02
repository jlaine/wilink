# Common definitions

QT += network xml
QNETIO_INCLUDE_DIR = $$PWD/src
QNETIO_LIBRARY_NAME = qnetio

# Libraries used internally by QNetIO
mac {
    QNETIO_INTERNAL_LIBS = -framework Foundation -framework Security
}

# Libraries for apps which use QNetIO
QNETIO_LIBS = -l$${QNETIO_LIBRARY_NAME}

# FIXME: we should be able to use the link_prl option to automatically pull in
# the extra libraries which our library needs, but this does not seem to work.
QNETIO_LIBS += $$QNETIO_INTERNAL_LIBS
