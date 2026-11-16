QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = client_qt
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    logindialog.cpp \
    registerdialog.cpp \
    changepassworddialog.cpp \
    browsefilmdialog.cpp \
    searchfilmdialog.cpp \
    bookticketdialog.cpp \
    viewticketsdialog.cpp \
    viewticketdetaildialog.cpp \
    admindialog.cpp \
    managerdialog.cpp \
    addshowtimedialog.cpp \
    ../lib/socket/socket.c \
    ../lib/messages/message.c

HEADERS += \
    mainwindow.h \
    logindialog.h \
    registerdialog.h \
    changepassworddialog.h \
    browsefilmdialog.h \
    searchfilmdialog.h \
    bookticketdialog.h \
    viewticketsdialog.h \
    viewticketdetaildialog.h \
    admindialog.h \
    managerdialog.h \
    addshowtimedialog.h \
    ../lib/socket/socket.h \
    ../lib/messages/message.h \
    ../client/headers/menu.h \
    ../lib/users/user.h \
    ../lib/loginedUser.h

INCLUDEPATH += \
    ../lib/socket \
    ../lib/messages \
    ../client/headers \
    ../lib/users \
    ../lib

# MySQL configuration
INCLUDEPATH += /usr/include/mysql
LIBS += -lmysqlclient

# Math library
LIBS += -lm

# Threading
LIBS += -pthread

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
