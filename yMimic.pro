QT += core gui widgets testlib

TARGET = yMimic

TEMPLATE = app

VPATH += src include
SOURCES += main.cc mainwindow.cc logger.cc repeater.cc

INCLUDEPATH += include
HEADERS += mainwindow.h logger.h repeater.h

FORMS   += mainwindow.ui

CONFIG += c++14

unix: LIBS += -lX11
