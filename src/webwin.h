#ifndef __WEBWINDOW_H__
#define __WEBWINDOW_H__

#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <webkit2/webkit2.h>

// https://developer.gnome.org/gtk3/stable/GtkWindow.html
// https://webkitgtk.org/reference/webkit2gtk/2.5.1/WebKitWebView.html

GtkWindow *webWindowCreate(void);

void webWindowLoadUri(GtkWindow *win, const gchar* uri);

void webWindowShow(GtkWindow *win);

void webWindowSetRect(GtkWindow *win, GdkRectangle* rect);

gboolean webWindowIsActive(GtkWindow *win);


#endif
   
