QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    wifiscanner.cpp

HEADERS += \
    mainwindow.h \
    wifiscanner.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    simpleble/dependencies/internal/fmt/LICENSE \
    simpleble/simpleble/include/simpleble_c/DEPRECATED \
    simpleble/simpleble/src_c/DEPRECATED

win32:LIBS += -lwlanapi -lshell32

TARGET = AIO_FW_Update_tool-V
VERSION = 1.0.0
TARGET = $$join(TARGET,,,$$VERSION)

