QT += core gui
QT += concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


# 使用SMore的sdk v3
INCLUDEPATH += G:\workData\company\SMore\ViMoCloud\sdk_3.14\include
LIBS +=        G:\workData\company\SMore\ViMoCloud\sdk_3.14\lib\vimo_inference.lib

# opencv
INCLUDEPATH += D:/Qt/opencv4.4.0/include
INCLUDEPATH += D:/Qt/opencv4.4.0/include/opencv2
CONFIG(release, debug|release){
LIBS += -LD:/Qt/opencv4.4.0/x64/vc15/lib/ -lopencv_world440
}
else{
LIBS += -LD:/Qt/opencv4.4.0/x64/vc15/lib/ -lopencv_world440d
}
