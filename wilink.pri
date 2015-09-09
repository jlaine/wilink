WILINK_VERSION = 2.4.903

EOL = $$escape_expand(\n\t)

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
mac {
    WILINK_LIB_PATH = $$WILINK_APP_PATH/wiLink.app/Contents/Frameworks
    WILINK_QML_PATH = $$WILINK_APP_PATH/wiLink.app/Contents/Resources/qml
} else {
    WILINK_LIB_PATH = $$WILINK_APP_PATH
    WILINK_QML_PATH = $$WILINK_APP_PATH
}
