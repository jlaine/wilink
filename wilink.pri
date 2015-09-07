WILINK_VERSION = 2.4.90

isEmpty(PREFIX) {
    unix {
        PREFIX = /usr/local
    }
}

WILINK_SOURCE_TREE = $$PWD
isEmpty(WILINK_BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$PWD),,
    WILINK_BUILD_TREE = $$clean_path($$OUT_PWD)
    WILINK_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}
WILINK_APP_PATH = $$WILINK_BUILD_TREE/bin
