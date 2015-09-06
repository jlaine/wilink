TEMPLATE = subdirs
isEmpty(WILINK_SYSTEM_MAD) {
    SUBDIRS += libmad
}
isEmpty(WILINK_SYSTEM_QXMPP) {
    SUBDIRS += qxmpp
}
