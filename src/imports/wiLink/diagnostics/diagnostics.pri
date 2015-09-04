HEADERS += \
    diagnostics/interface.h \
    diagnostics/QXmppDiagnosticIq.h \
    diagnostics/network.h \
    diagnostics/software.h \
    diagnostics/transfer.h \
    diagnostics/wireless.h

SOURCES += \
    diagnostics/interface.cpp \
    diagnostics/QXmppDiagnosticIq.cpp \
    diagnostics/network.cpp \
    diagnostics/software.cpp \
    diagnostics/transfer.cpp \
    diagnostics/wireless.cpp

android {
    # FIXME: this is a hack so that Q_OS_ANDROID is defined
    DEFINES += ANDROID
    SOURCES += diagnostics/wireless_stub.cpp
} else:mac {
    OBJECTIVE_SOURCES += diagnostics/wireless_mac.mm
    LIBS += -framework CoreWLAN -framework AppKit
} else:unix {
    QT += dbus
    SOURCES += diagnostics/wireless_nm.cpp
} else {
    SOURCES += diagnostics/wireless_stub.cpp
}
