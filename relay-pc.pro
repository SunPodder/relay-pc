QT += core widgets network dbus

CONFIG += c++17

TARGET = relay-pc
TEMPLATE = app

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
    src/main.cpp \
    src/MainWindow.cpp \
    src/NotificationPanel.cpp \
    src/NotificationCard.cpp \
    src/NotificationManager.cpp \
    src/AnimationManager.cpp \
    src/NotificationData.cpp \
    src/NotificationPopup.cpp \
    src/NotificationPopupManager.cpp

HEADERS += \
    src/MainWindow.h \
    src/NotificationPanel.h \
    src/NotificationCard.h \
    src/NotificationManager.h \
    src/AnimationManager.h \
    src/NotificationData.h \
    src/NotificationPopup.h \
    src/NotificationPopupManager.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
