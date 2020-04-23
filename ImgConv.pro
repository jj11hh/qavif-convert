#-------------------------------------------------
#
# Project created by QtCreator 2020-04-19T18:15:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ImgConv
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        convworker.cpp \
        dialogsettings.cpp \
        exifdump.cpp \
        imgconvsettings.cpp \
        jpegavifconverter.cpp \
        jpegsegreader.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        convworker.h \
        dialogsettings.h \
        exifdump.h \
        imgconvsettings.h \
        jpegavifconverter.h \
        jpegsegreader.h \
        mainwindow.h

FORMS += \
        dialogsettings.ui \
        mainwindow.ui

DESTDIR = bin

LIBS += -L"$$_PRO_FILE_PWD_/thirdparty/libavif/" -lavif -lrav1e -ldav1d -laom
LIBS += -L"$$_PRO_FILE_PWD_/thirdparty/libjpeg-turbo-2.0.4/lib" -lturbojpeg

INCLUDEPATH += $$_PRO_FILE_PWD_/thirdparty/libavif/include/avif
INCLUDEPATH += $$_PRO_FILE_PWD_/thirdparty/libjpeg-turbo-2.0.4/include/


RC_ICONS = Images/icon.ico

win32-msvc* {
    QMAKE_CXXFLAGS += /utf-8
    LIBS += -lws2_32 -ladvapi32 -lpsapi -lUserenv
}

unix {
    LIBS += -ldl
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    icons.qrc
