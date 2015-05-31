#-------------------------------------------------
#
# Project created by QtCreator 2013-03-14T23:34:23
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qRedmine
TEMPLATE = app

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/redminerequest.cpp \
    src/issuesmodel.cpp \
    src/issuesortfilterproxymodel.cpp \
    src/settingsdialog.cpp \
    src/redmine.cpp \
    src/progressdialog.cpp \
    src/timeconfirmationdialog.cpp \
    src/about.cpp

HEADERS  += src/mainwindow.h \
    src/redminerequest.h \
    src/issuesmodel.h \
    src/issuesortfilterproxymodel.h \
    src/settingsdialog.h \
    src/globals.h \
    src/redmine.h \
    src/progressdialog.h \
    src/timeconfirmationdialog.h \
    src/about.h

FORMS    += src/mainwindow.ui \
    src/settingsdialog.ui \
    src/issueactivitydialog.ui \
    src/progressdialog.ui \
    src/timeconfirmation.ui \
    src/about.ui

OTHER_FILES += \
    data/style.css

RESOURCES += \
    data/application.qrc

ICON = data/images/redmine_icon.icns

DEFINES += PER_MONTH_HOURS_GOAL=\\\"152.5\\\"
