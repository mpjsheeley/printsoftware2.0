QT       += core gui network printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 openssl-linked

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    downloaddialog.cpp \
    main.cpp \
    mainwindow.cpp \
    portraitrectitem.cpp \
    portraitscene.cpp \
    portraitview.cpp \
    printdialog.cpp \
    settingsdialog.cpp \
    simpleprintdialog.cpp

HEADERS += \
    aboutdialog.h \
    downloaddialog.h \
    mainwindow.h \
    portraitrectitem.h \
    portraitscene.h \
    portraitview.h \
    printdialog.h \
    settingsdialog.h \
    simpleprintdialog.h

FORMS += \
    aboutdialog.ui \
    downloaddialog.ui \
    mainwindow.ui \
    printdialog.ui \
    settingsdialog.ui \
    simpleprintdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
