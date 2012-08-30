TEMPLATE = lib
CONFIG += staticlib
HEADERS = qtlocalpeer.h qtlockedfile.h
SOURCES = qtlocalpeer.cpp qtlockedfile.cpp
unix:SOURCES += qtlockedfile_unix.cpp
else:win32:SOURCES += qtlockedfile_win.cpp
