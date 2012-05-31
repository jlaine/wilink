WILINK_INCLUDE_DIR = $$PWD/src
WILINK_VERSION = 2.3.0

isEmpty(PREFIX) {
    contains(MEEGO_EDITION,harmattan) {
        PREFIX = /usr
    } else:unix {
        PREFIX = /usr/local
    }
}
