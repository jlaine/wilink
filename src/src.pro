TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    idle \
    qnetio \
    qsound \
    qxmpp-extra

SUBDIRS += app
CONFIG += ordered
