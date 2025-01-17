
QT -= core gui
TARGET = highlight
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

win32 {
DESTDIR = ..\\..\\..
SOURCES += ..\\..\\cli\\main.cpp \
    ..\\..\\cli\\help.cpp \
    ..\\..\\cli\\cmdlineoptions.cpp \
    ..\\..\\cli\\arg_parser.cc
HEADERS += ..\\..\\cli\\main.h \
    ..\\..\\cli\\help.h \
    ..\\..\\cli\\cmdlineoptions.h \
    ..\\..\\cli\\arg_parser.h

INCLUDEPATH += E:\\devel\\cpp\\lua_bin_5.3.0
#INCLUDEPATH +=D:/Devel/cpp/LuaJIT-2.0.0/src

INCLUDEPATH += E:\\devel\\cpp\\boost_1_70_0
INCLUDEPATH += ..\\..\\include
QMAKE_CXXFLAGS += -std=c++11
}
else {
SOURCES += cli/main.cpp \
    cli/help.cpp \
    cli/cmdlineoptions.cpp \
    cli/arg_parser.cc
HEADERS += src/cli/main.h \
    cli/help.h \
    cli/cmdlineoptions.h \
    cli/arg_parser.h
}

win32:LIBS += -L../../ -lhighlight
unix:LIBS += -L.. -lhighlight

win32 {
    contains(QT_ARCH, i386) {
        LIBS += -LE:\Devel\cpp\lua_bin_5.3.0 -llua
    } else {
        LIBS += -LE:\Devel\cpp\lua_bin_5.3.0_x64 -llua
    }
}

!contains( LINKTYPE, LUA52 ) {
unix:LIBS += -L.. -llua5.1
}
contains( LINKTYPE, LUA52 ) {
unix:LIBS += -L.. -llua5.2
unix:DEFINES += USE_LUA52
}

win32:QMAKE_POST_LINK = E:\Devel\upx393w\upx.exe --best ..\..\..\highlight.exe
