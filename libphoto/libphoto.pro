QT       += core network
TEMPLATE = lib
DEFINES += LIBPHOTO_LIBRARY

include (../config.pro)

#in
# version 1.1.0
# add video capture

SOURCES += phototcp.cpp photohttp.cpp

HEADERS += phototcp.h photohttp.h \
           request_photo.h \ 
           libphoto_global.h


#out
VER_MAJ = 1
VER_MIN = 1
VER_PAT = 0
TARGET = $$qtLibraryTarget(photo)
DESTDIR = ../$${CURRENT_BUILD}
DLLDESTDIR = ../$${RELEASEDIR}

QMAKE_LFLAGS += -Wl,-allow-multiple-definition

MOC_DIR = moc
OBJECTS_DIR = obj