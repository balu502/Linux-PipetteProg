#-------------------------------------------------
#
# Project created by QtCreator 2012-03-07T09:35:45
#
#-------------------------------------------------

VERSION = 5.00.00
#2.10.00 QtScrVersion
#2.20.00 qt+python +pause bth
#2.21.00 2.20 remove canReInit
#2.30.00 2.21 +sets.setError(int) signalisation function
#2.31.00 2.30 + cover open/pause error remove
#remove library libcarm
#3.00.00 add user menu on run time
#scaners define in settings.xml
#3.00.02 UV light turn off timer add
#3.10.00 pause script on cover open eneble/disable from settings function add
#3.20.00 27/02.20 Add procedure initialisation of display on go back from wait regime
#3.30.00 14.05.20 Log UV lamp logic
#4.00.00 14.05.20 Add photo function.Use library linphoto.so.1.0.0
#4.10.00 11.09.20 Add video capture function. Use library linphoto.so.1.1.0
#4.10.01 02.06.21 Add ,. in scaner char set
#5.00.00 29.12.21 try refactoring of project. Add control version
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QT       += core network script

QT       -= gui

MOC_DIR = moc
OBJECTS_DIR = obj

include (../config.pro)
DESTDIR = ../$${CURRENT_BUILD}
CONFIG   += console
CONFIG   -= app_bundle

TARGET = ExecMod
target.files = TARGET
target.path = /data/ExecPS
INSTALLS += target
export(INSTALLS)
TEMPLATE = app

#LIBS += -L/home/dnadev/sam/prog/qt/lwlib2/ -lcarm
PYTHON_VERSION=2.6
INCLUDEPATH += /home/dnadev/gusev/pythonqt-2.1-src/Python-2.6.6/angstromP26/usr/include/python2.6
INCLUDEPATH += /home/dnadev/gusev/pythonqt-2.1-src/PythonQt2.1_Qt4.6.2/src
INCLUDEPATH += ../libphoto
LIBS += -L/home/dnadev/gusev/pythonqt-2.1-src/PythonQt2.1_Qt4.6.2/lib/ -lPythonQt
LIBS += -L../$${CURRENT_BUILD} -lphoto

SOURCES += main.cpp \
    commobject.cpp userinterface.cpp display.cpp can_commsock.c\
    logobject.cpp \
    settings.cpp \
    codescanobject.cpp \
    ProgramExecuteThread.cpp \
    CommThread.cpp \
    uithread.cpp \
    chreader.cpp \
    ioqtscr.cpp \
    photoqtscr.cpp

HEADERS += \
    ProgramExecuteThread.h \
   display.h can_comm.h can_ids.h \
    logobject.h \
    settings.h \
    codescanobject.h \
    userinterface.h \
    lcd.h \
    CommThread.h \
    commobject.h \
    uithread.h \
    chreader.h \
    ioqtscr.h \
    photoqtscr.h

