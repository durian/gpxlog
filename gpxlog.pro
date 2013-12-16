QT -= core gui
TARGET = gpxlog.xpl
TEMPLATE = lib

CONFIG += plugin release warn_on
CONFIG -= qt


#QMAKE_CXX = /usr/local/bin/g++-4.7

INCLUDEPATH += ../SDK/CHeaders/XPLM
INCLUDEPATH += ../SDK/CHeaders/Wrappers
INCLUDEPATH += ../SDK/CHeaders/Widgets

#SDK/Libraries/Mac


QMAKE_CXXFLAGS += -fPIC
QMAKE_LFLAGS += -shared -fPIC

# Defined to use X-Plane SDK 2.0 capabilities - no backward compatibility before 9.0
DEFINES += XPLM200

CONFIG   += console warn_on release shared
CONFIG   -= app_bundle

CONFIG(debug, debug|release) {
    # Debug
    message("Debug Build")
} else {
    # Release
    message("Release Build")
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
}

win32 {
    DEFINES += LIN=0 APL=0 IBM=1
    TARGET = win.xpl
}
unix:!macx {
    DEFINES += LIN=1 APL=0 IBM=0
    TARGET = lin.xpl
}
macx {
    DEFINES += LIN=0 APL=1 IBM=0
    QMAKE_LFLAGS += -dynamiclib
    #QMAKE_LFLAGS += -flat_namespace #-undefined supress
    QMAKE_LFLAGS += -F../SDK/Libraries/Mac

    LIBS += -framework XPLM -framework CoreFoundation
    TARGET = mac.xpl
    CONFIG += x64_86
    CONFIG += x86
}

#LIBS += -lalut -lopenal

SOURCES += gpxlog.cpp

HEADERS += gpxlog.h
