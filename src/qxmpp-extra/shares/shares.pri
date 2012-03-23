QT += sql

INCLUDEPATH += ../../qdjango/src/db
LIBS += -L../qdjango/src/db -lqdjango-db

HEADERS += \
    shares/QXmppShareDatabase.h \
    shares/QXmppShareDatabase_p.h \
    shares/QXmppShareIq.h \
    shares/QXmppShareManager.h

SOURCES += \
    shares/QXmppShareDatabase.cpp \
    shares/QXmppShareIq.cpp \
    shares/QXmppShareManager.cpp
