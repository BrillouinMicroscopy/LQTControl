# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

TEMPLATE = app
TARGET = FPIControl
DESTDIR = ./debug
QT += core widgets gui charts
CONFIG += debug
DEFINES += _WINDOWS WIN64 QT_DEPRECATED_WARNINGS QT_WIDGETS_LIB
INCLUDEPATH += . \
    ./debug \
    . \
    $(QTDIR)/mkspecs/win32-msvc2015 \
    ./GeneratedFiles
LIBS += -lshell32 -luser32
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/debug
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
HEADERS += ./mainwindow.h
SOURCES += ./main.cpp \
    ./mainwindow.cpp
FORMS += ./mainwindow.ui
