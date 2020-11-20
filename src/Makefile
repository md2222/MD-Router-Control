#!/bin/make
#Makefile curl1

SOURCES = main.cpp settings.cpp prefdialog.cpp webview.cpp

CFLAGS  = `pkg-config --cflags gtk+-3.0`
#CFLAGS += -I../../api/

LIBS    = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gnome-keyring-1`
LIBS    += -I/usr/include/webkitgtk-4.0 
LIBS    += -I/usr/include/libsoup-2.4

TARGET  = mdrctrl


all:
	g++ $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)
