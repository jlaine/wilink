TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    idle \
    qdjango \
    qnetio \
    qsound \
    qxmpp

SUBDIRS += plugins
CONFIG += ordered
