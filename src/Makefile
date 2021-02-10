#!/bin/make

SOURCES = main.cpp prefdialog.cpp settings.cpp webview.cpp

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
	mkdir -p -- "$(DESTDIR)$(PREFIX)/share/mdrctrl"
	mkdir -p -- "$(DESTDIR)$(PREFIX)/share/mdrctrl/icons"
	cp -- mdrctrl "$(DESTDIR)$(PREFIX)/bin/"
	cp -- settings.ui "$(DESTDIR)$(PREFIX)/share/mdrctrl/"
	cp -- icons/*.png "$(DESTDIR)$(PREFIX)/share/mdrctrl/icons/"
	cp -- mdrctrl.desktop "$(DESTDIR)$(PREFIX)/share/applications"
	cp -- icons/mdrctrl.png /usr/share/pixmaps
	
	
uninstall:
	-rm -- "$(DESTDIR)$(PREFIX)/bin/mdrctrl"
	-rm -r -- "$(DESTDIR)$(PREFIX)/share/mdrctrl"
	-rm -- "$(DESTDIR)$(PREFIX)/share/applications/mdrctrl.desktop"
	-rm -- /usr/share/pixmaps/mdrctrl.png
	
	
clean:
	-rm -f -- *.o mdrctrl	

.PHONY: all install uninstall clean
