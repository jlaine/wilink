TEMPLATE = subdirs

SUBDIRS = \
    idle \
    qdjango \
    qnetio \
    qsound \
    qxmpp

!symbian {
    SUBDIRS += translations
}

SUBDIRS += plugins
CONFIG += ordered
