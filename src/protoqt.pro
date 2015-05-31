#-------------------------------------------------
#
# Project created by QtCreator 2013-03-14T23:34:23
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = protoqt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    redminerequest.cpp \
    issuesmodel.cpp \
    issuesortfilterproxymodel.cpp \
    settingsdialog.cpp \
    redmine.cpp \
    progressdialog.cpp \
    timeconfirmationdialog.cpp \
    about.cpp

HEADERS  += mainwindow.h \
    redminerequest.h \
    issuesmodel.h \
    issuesortfilterproxymodel.h \
    settingsdialog.h \
    globals.h \
    redmine.h \
    progressdialog.h \
    timeconfirmationdialog.h \
    about.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    issueactivitydialog.ui \
    progressdialog.ui \
    timeconfirmation.ui \
    about.ui

OTHER_FILES += \
    style.css

RESOURCES += \
    application.qrc

ICON = images/redmine_icon.icns

DEFINES += PER_MONTH_HOURS_GOAL=\\\"152.5\\\"
