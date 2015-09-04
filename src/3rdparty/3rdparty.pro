TEMPLATE = subdirs
SUBDIRS = libmad qtsingleapplication
isEmpty(WILINK_SYSTEM_QXMPP) {
    SUBDIRS += qxmpp
}
