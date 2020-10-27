DESTDIR = .
TEMPLATE = app
TARGET = FEBioStudioUpdater
DESTDIR = ./build/bin
CONFIG += debug c++14
CONFIG += qt opengl qtwidgets qtcharts warn_off
DEFINES += LINUX
INCLUDEPATH += .
QT += widgets opengl gui charts network webenginewidgets
QMAKE_CXX = g++
QMAKE_CXXFLAGS += -std=c++14 -O0
QMAKE_LFLAGS += -static-libstdc++ -static-libgcc
#QMAKE_LFLAGS_RPATH = \$$ORIGIN/../lib/
QMAKE_RPATHDIR += $ORIGIN/../lib
#QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../lib/\',-z,origin'

LIBS += -L/home/mherron/Resources/Qt/5.13.2/gcc_64/lib
LIBS += -L/usr/local/lib 
LIBS += -Wl,--start-group

LIBS += -L/usr/lib64

#RESOURCES = ../febiostudio.qrc

HEADERS += *.h
SOURCES = $$files(*.cpp)
SOURCES -= $$files(moc_*.cpp)
SOURCES -= $$files(qrc_*)


