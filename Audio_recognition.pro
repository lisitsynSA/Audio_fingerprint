#-------------------------------------------------
#
# Project created by QtCreator 2015-09-20T16:41:58
#
#-------------------------------------------------

QT       += core gui multimedia printsupport

win32 {
    RC_FILE += file.rc
    OTHER_FILES += file.rc
}

TARGET = Audio_recognition
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    audioinfo_t.cpp \
    qcustomplot.cpp \
    wave_t.cpp

HEADERS  += mainwindow.h \
    audioinfo_t.h \
    qcustomplot.h \
    fftw3.h \
    wave_t.h

FORMS    += mainwindow.ui

win32: LIBS += -L$$PWD/ -llibfftw3f-3

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

OTHER_FILES += \
    file.rc
