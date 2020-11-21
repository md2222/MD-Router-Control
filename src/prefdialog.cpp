#include "prefdialog.h"


PrefDialog::PrefDialog(GtkWidget *parent, const char* uiDir)
{
    rect.x = 500;  rect.y = 300;  rect.w = 0;  rect.h = 0;

    uiPath = g_strnfill(MAX_PATH, '\0');
    strcat(uiPath, uiDir);
    strcat(uiPath, "settings.ui");
    //createDialog();
};


PrefDialog::~PrefDialog()
{
    gtk_widget_destroy(GTK_WIDGET(window));
};


void PrefDialog::createDialog()
{
    window = NULL;
    GtkBuilder *builder = gtk_builder_new();

    if (gtk_builder_add_from_file(builder, uiPath, &error) == 0)
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    window = GTK_WIDGET (gtk_builder_get_object(builder, "dlgOptions"));
    gtk_window_set_title(GTK_WINDOW (window), "Options");
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(&onCloseCb), this);  //++
    gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);
    g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(&onConfigureCb), this);

    btOk = GTK_WIDGET(gtk_builder_get_object(builder, "btOk"));
    g_signal_connect(btOk, "clicked", G_CALLBACK(&onOkCb), this);

    btCancel = GTK_WIDGET(gtk_builder_get_object(builder, "btCancel"));
    g_signal_connect(btCancel, "clicked", G_CALLBACK(&onCancelCb), this);

    edAddr = (GtkEntry*)gtk_builder_get_object(builder, "edAddr");
    edUser = (GtkEntry*)gtk_builder_get_object(builder, "edUser");
    edPassw = (GtkEntry*)gtk_builder_get_object(builder, "edPassw");
    edTestAddr = (GtkEntry*)gtk_builder_get_object(builder, "edTestAddr");

    g_object_unref(G_OBJECT (builder));
}


void PrefDialog::setRect(Rect* r)
{
    if (r->x * r->y > 0)
    {
        //gtk_window_move(GTK_WINDOW(window), r->x, r->y);
        rect = *r;
    }
}


Rect PrefDialog::getRect()
{
    return rect;
}


void PrefDialog::show()
{
    createDialog();

    if (!window)
    {
        printf("Error creating settings dialog\n");
        return;
    }

    //printf("PrefDialog::show:    %d    %d\n",rect.x, rect.y);
    gtk_window_move(GTK_WINDOW(window), rect.x, rect.y);

    gtk_entry_set_text(edAddr, servAddr->data());
    gtk_entry_set_text(edUser, user->data());
    // after show passw is empty
    if (passw)
        gtk_entry_set_text(edPassw, passw);
    gtk_entry_set_text(edTestAddr, testAddr->data());

    gtk_widget_show_all(window);
}

/*
void PrefDialog::hide()
{
    gtk_widget_hide(window);
}
*/

gboolean PrefDialog::onConfigure(GdkEvent *event)
{
    //rect.x = event->configure.x;
    //rect.y = event->configure.y;
    gtk_window_get_position(GTK_WINDOW(window), &rect.x, &rect.y);
    //printf("PrefDialog::onConfigure:    %d    %d\n",rect.x, rect.y);
    return FALSE;
}


void PrefDialog::procPassw(bool save)
{
    //printf("PrefDialog::procPassw\n");
    const gchar *text = gtk_entry_get_text(edPassw);
    if (text)
    {
        if (save)
            savePassw(text);

        gchar *zero = g_strnfill(strlen(text), '*');
        gtk_entry_set_text(edPassw, zero);
        g_free(zero);
    }
}


void PrefDialog::onOk()
{
    const gchar *text = gtk_entry_get_text(edAddr);
    if (text)  servAddr->assign(text);

    text = gtk_entry_get_text(edUser);
    if (text)  user->assign(text);

    text = gtk_entry_get_text(edTestAddr);
    if (text)  testAddr->assign(text);

    procPassw(true);

    //gtk_widget_hide(window);
    gtk_widget_destroy(window);
}


void PrefDialog::onCancel()
{
    procPassw(false);
    //gtk_widget_hide(window);
    gtk_widget_destroy(window);
}


gboolean PrefDialog::onClose()
{
    procPassw(false);
    //return gtk_widget_hide_on_delete(window);
    return FALSE;
}
