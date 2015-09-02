WILINK_INCLUDE_DIR = $$PWD/src
WILINK_VERSION = 2.4.90

isEmpty(PREFIX) {
    unix {
        PREFIX = /usr/local
    }
}
