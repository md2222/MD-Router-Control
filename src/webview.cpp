#include "webview.h"


WebView::WebView()
{
    winRect.x = 200;  winRect.y = 100;  winRect.w = 1280;  winRect.h = 900;

    g_autoptr(GError) err = NULL;
    winIcon = gdk_pixbuf_new_from_resource ("/app/icons/mdrctrl.png", NULL);
    if (!winIcon)
        g_warning ("Load window icon error: %s\n", err->message);
};


bool WebView::isActive()
{
    return winWeb != NULL;
}


gboolean WebView::onKeyPress(GdkEventKey *event)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        close();
        return TRUE;
    }
    return FALSE;
}


void WebView::close()
{
    gtk_window_close(winWeb);
    //onClose();
}


void WebView::setRect(Rect* rect)
{
    winRect = *rect;
}


void WebView::setRect()
{
    if (winRect.x * winRect.y > 0)
        gtk_window_move(winWeb, winRect.x, winRect.y);
    if (winRect.w * winRect.h > 0)
        gtk_window_set_default_size(winWeb, winRect.w, winRect.h);
}


Rect WebView::getRect()
{
    return winRect;
}


gboolean WebView::onClose()
{
    gtk_window_get_position(winWeb, &winRect.x, &winRect.y);
    gtk_window_get_size(winWeb, &winRect.w, &winRect.h);

    gtk_widget_destroy(GTK_WIDGET(web));
    web = NULL;
    gtk_widget_destroy(GTK_WIDGET(winWeb));
    winWeb = NULL;

    g_idle_add(httpPingLater, NULL);

    return TRUE;
}


void WebView::loadUrl(const char* url)
{
    if (isActive())  close();

    winWeb = (GtkWindow *)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(winWeb), 1280, 900);
    gtk_window_set_title(GTK_WINDOW(winWeb), "MD Router Control");

    if (winIcon)
        gtk_window_set_icon (winWeb, winIcon);

    g_signal_connect(winWeb, "delete-event", G_CALLBACK(onCloseCb), this);
    g_signal_connect_after(winWeb, "key_press_event", G_CALLBACK(onKeyPressCb), this);

    // Websites will not store any data in the client storage. This is normally used to implement private instances.
    WebKitWebContext *context = webkit_web_context_new_ephemeral();
    // This process model is indicated for applications which may use a number of web views
    // ... for example a full-fledged web browser with support for multiple tabs.
    //webkit_web_context_set_process_model(context, WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);

    web = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));

    gtk_container_add(GTK_CONTAINER(winWeb), GTK_WIDGET(web));

    setRect();

    webkit_web_view_load_uri(web, url);

    gtk_widget_grab_focus(GTK_WIDGET(web));
    gtk_widget_show_all(GTK_WIDGET(winWeb));
}


void WebView::show()
{
    if (isActive())
        gtk_window_present(winWeb);
}

//----------------------------------------------------------------------------------------------------------------------
