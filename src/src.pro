TEMPLATE = subdirs

SUBDIRS = \
    idle \
    qdjango \
    qnetio \
    qsound \
    qxmpp

SUBDIRS += plugins
CONFIG += ordered
