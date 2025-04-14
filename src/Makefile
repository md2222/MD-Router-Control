#!/bin/make

SOURCES = resources.c conffile.c webwin.c confdialog.c main.c

CFLAGS  = `pkg-config --cflags gtk+-3.0`
#CFLAGS += -I../../api/

INC=-I/usr/include/libsecret-1

#LIBS    = `pkg-config --libs gtk+-3.0`
LIBS    = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 libsecret-1`
LIBS    += -I/usr/include/webkitgtk-4.0 
LIBS    += -I/usr/include/libsoup-2.4

TARGET  = mdrctrl


all:
	glib-compile-resources --generate-source resources.xml
#	g++ $(CFLAGS) $(INC) $(SOURCES) -o $(TARGET) $(LIBS)
	gcc $(CFLAGS) $(INC) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
			rm -rf mdrctrl *.o
