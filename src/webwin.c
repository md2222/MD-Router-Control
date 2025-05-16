#include "webwin.h"



extern gchar* appName;
GdkRectangle defRect = { 200, 100, 1280, 900 };


static gboolean onKeyPress(GtkWidget* win, GdkEventKey* event, gpointer user_data)
{
    //g_print("onKeyPress:    %s\n", gdk_keyval_name(event->keyval));

    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_window_close(GTK_WINDOW(win));

        return TRUE;
    }
    
    return FALSE;
}


GtkWindow *webWindowCreate(void)
{
    GtkWindow *win = (GtkWindow *)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), defRect.width, defRect.height);

    g_signal_connect_after(win, "key_press_event", G_CALLBACK(onKeyPress), NULL);

    // Websites will not store any data in the client storage. This is normally used to implement private instances.
    WebKitWebContext *context = webkit_web_context_new_ephemeral();

    GtkWidget *web = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));

    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(web));

    gtk_widget_grab_focus(GTK_WIDGET(web));
    gtk_widget_show_all(GTK_WIDGET(win));
    
    return win;
}


void webWindowSetRect(GtkWindow *win, GdkRectangle* rect)
{
    if (!win || !rect)  return;
    
    if (rect->x * rect->y > 0)
        gtk_window_move(win, rect->x, rect->y);
    if (rect->width * rect->height > 0)
        gtk_window_resize(win, rect->width, rect->height);
}


GtkWidget *webWindowGetWeb(GtkWindow *win)
{
    if (!win)  return NULL;
    
    GtkWidget *web = gtk_container_get_focus_child (GTK_CONTAINER(win));
    
    return web;
}


void webWindowLoadUri(GtkWindow *win, const gchar* uri)
{
    if (!win || !uri)  return;

    GtkWidget *web = webWindowGetWeb(win);

    webkit_web_view_load_uri(web, uri);
}


void webWindowShow(GtkWindow *win)
{
    if (!win)  return;
    
    gtk_window_present(win);
}


gboolean webWindowIsActive(GtkWindow *win)
{
    if (!win)  return FALSE;
    
    return gtk_window_is_active (win);
}
