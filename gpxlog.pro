TEMPLATE = lib
QT -= gui core

CONFIG += warn_on plugin release
CONFIG -= thread exceptions qt rtti debug
CONFIG -= app_bundle
CONFIG -= qt core

VERSION = 1.0.0

#QMAKE_CXX = /usr/local/bin/g++-4.7

INCLUDEPATH += ../SDK/CHeaders/XPLM
INCLUDEPATH += ../SDK/CHeaders/Wrappers
INCLUDEPATH += ../SDK/CHeaders/Widgets

QMAKE_CXXFLAGS += -fPIC
QMAKE_LFLAGS += -shared -fPIC

DEFINES += XPLM200

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
    CONFIG += x64_86
    CONFIG += x86
    #just trying QMAKE_LFLAGS += -F.
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

SOURCES += gpxlog.cpp Info.cpp

HEADERS += gpxlog.h Info.h
