#!/bin/make

SOURCES = main.cpp prefdialog.cpp settings.cpp webview.cpp resources.c

CFLAGS  = `pkg-config --cflags gtk+-3.0`
#CFLAGS += -I../../api/

LIBS    = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gnome-keyring-1`
LIBS    += -I/usr/include/webkitgtk-4.0 
LIBS    += -I/usr/include/libsoup-2.4

TARGET  = mdrctrl


all:
	g++ $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)
	

PREFIX=/usr/local

#	which mdrctrl && { echo ! mdrctrl already installed, first uninstall old version; exit 1; }  || echo installing...
install: mdrctrl
	cp -- mdrctrl "$(DESTDIR)$(PREFIX)/bin/"
	cp -- mdrctrl.desktop "/usr/share/applications/"
	cp -- mdrctrl.png "/usr/share/pixmaps/"
	
	
uninstall:
	-rm -- "$(DESTDIR)$(PREFIX)/bin/mdrctrl"
	-rm -- "/usr/share/applications/mdrctrl.desktop"
	-rm -- "/usr/share/pixmaps/mdrctrl.png"
	
	
clean:
	-rm -f -- *.o mdrctrl	

.PHONY: all install uninstall clean
