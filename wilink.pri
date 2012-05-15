WILINK_INCLUDE_DIR = $$PWD/src
WILINK_VERSION = 2.2.908

isEmpty(PREFIX) {
    contains(MEEGO_EDITION,harmattan) {
        PREFIX = /usr
    } else:unix {
        PREFIX = /usr/local
    }
}
