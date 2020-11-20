#ifndef __WEBVIEW_H__
#define __WEBVIEW_H__

#include <gtk/gtk.h>
#include <string>
#include <gtk/gtkwindow.h>
#include <webkit2/webkit2.h>
#include "main.h"

using namespace std;


// https://developer.gnome.org/gtk3/stable/GtkWindow.html
// https://webkitgtk.org/reference/webkit2gtk/2.5.1/WebKitWebView.html

class WebView
{
public:
    GtkWindow *winWeb = 0;
    GdkPixbuf *winIcon;
    WebView()
    {
        winRect.x = 200;  winRect.y = 100;  winRect.w = 1280;  winRect.h = 900;
    };
    ~WebView()
    {
    };
    void setIcon(const char* fileName);
    void loadUrl(const char* url);
    void show();
    void setRect(Rect* rect);
    Rect getRect();
    void close();
    bool isActive();

private:
    WebKitWebView *web = 0;
    Rect winRect;
    void setRect();
    gboolean onClose();
    static gboolean onCloseCb(GtkWidget *widget, GdkEvent *event, gpointer data)
    {
        return ((WebView*)data)->onClose();
    };
    static gboolean onKeyPressCb(GtkWindow *window, GdkEventKey *event, gpointer data)
    {
        return ((WebView*)data)->onKeyPress(event);
    };
    gboolean onKeyPress(GdkEventKey *event);
};


#endif
