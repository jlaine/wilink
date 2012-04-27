TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    qnetio \
    qsound \
    qxmpp-extra

SUBDIRS += app imports
CONFIG += ordered
