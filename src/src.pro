TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    idle \
    qdjango \
    qnetio \
    qsound \
    qxmpp \
    qxmpp-extra

SUBDIRS += app
CONFIG += ordered
