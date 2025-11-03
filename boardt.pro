QT += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

install_it.path = $$OUT_PWD
install_it.files = config/*

INSTALLS += \
    install_it

SOURCES += \
    src/board/board.cpp \
    src/board_widget/boardwidget.cpp \
    src/config_reader/configreader.cpp \
    src/main.cpp \
    src/core/mainwindow.cpp \
    src/logger/logger.cpp

HEADERS += \
    src/board/board.h \
    src/board_widget/boardwidget.h \
    src/config_reader/configreader.h \
    src/core/mainwindow.h \
    src/logger/logger.h

FORMS += \
    form/mainwindow.ui \
    src/board_widget/boardwidget.ui

INCLUDEPATH += ./src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# install_it.path = $$OUT_PWD/debug
# install_it.files = config/*

# INSTALLS += \
#     install_it


