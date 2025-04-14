#include "confdialog.h"


static GtkWidget  *window = NULL;
static GdkRectangle defWinRect = { 500, 300, 271, 200 };  // кнопки прилеплены к низу
static GError *error = NULL;


void procPassw(ConfigDialog* dlg, gboolean save)
{
    const gchar *text = gtk_entry_get_text(dlg->edPassw);
    if (text)
    {
        if (save)
            savePassw(text);

        gchar *zero = g_strnfill(strlen(text), '*');
        gtk_entry_set_text(dlg->edPassw, zero);
        g_free(zero);
    }
}


static
gboolean onClose(GtkWidget *win, GdkEvent *ev, ConfigDialog* dlg)
{
    procPassw(dlg, FALSE);
    
    GdkRectangle* rect = &dlg->rect;
    gtk_window_get_position(GTK_WINDOW(win), &rect->x, &rect->y);
    gtk_window_get_size(GTK_WINDOW(win), &rect->width, &rect->height);

    g_free(dlg);

    return FALSE;
}

/*
static
gboolean onConfigure(GdkEvent *event)
{
    //rect.x = event->configure.x;
    //rect.y = event->configure.y;
    gtk_window_get_position(GTK_WINDOW(window), &prefWinRect.x, &prefWinRect.y);
    //printf("PrefDialog::onConfigure:    %d    %d\n",rect.x, rect.y);
    return FALSE;
}
*/

static
void onOk(GtkWidget *wgt, ConfigDialog* dlg)
{
    ConfigData* data = &dlg->data;
    
    // This string points to internally allocated storage in the widget and must not be freed, modified or stored.
    const gchar *text = gtk_entry_get_text(dlg->edAddr);
    if (text)  
    {
        g_free(data->servAddr);
        data->servAddr = g_strdup(text);
    }

    text = gtk_entry_get_text(dlg->edUser);
    if (text) 
    {
        g_free(data->user);
        data->user = g_strdup(text);
    }

    text = gtk_entry_get_text(dlg->edTestAddr);
    if (text) 
    {
        g_free(data->testAddr);
        data->testAddr = g_strdup(text);
    }

    procPassw(dlg, TRUE);
    
    dlg->lastResult = TRUE;
    dlg->onOk();  // can works too
    
    gtk_window_close(dlg->window);
}


static
void onCancel(GtkWidget *wgt, ConfigDialog* dlg)
{
    procPassw(dlg, FALSE);
    
    dlg->lastResult = FALSE;
    
    gtk_window_close(dlg->window);
}


ConfigDialog* configDialogCreate()
{
    ConfigDialog* dlg = g_malloc(sizeof(ConfigDialog));

    dlg->rect = defWinRect;  
    dlg->lastResult = FALSE;

    GtkBuilder *builder = gtk_builder_new();

    if (gtk_builder_add_from_resource(builder, "/app/settings.ui", &error) == 0)
    {
        g_printerr("Error loading ui file: %s\n", error->message);
        g_clear_error(&error);
        return 0;
    }

    GtkWidget *window = GTK_WIDGET (gtk_builder_get_object(builder, "dlgOptions"));
    dlg->window = window;
    gtk_window_set_title(GTK_WINDOW (window), "Options");
    gtk_window_set_default_size(GTK_WINDOW(window), defWinRect.width, defWinRect.height);
    gtk_window_set_resizable(GTK_WINDOW (window), FALSE);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(onClose), dlg); 
    //g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(onConfigure), NULL);

    dlg->btOk = GTK_WIDGET(gtk_builder_get_object(builder, "btOk"));
    g_signal_connect(dlg->btOk, "clicked", G_CALLBACK(onOk), dlg);

    dlg->btCancel = GTK_WIDGET(gtk_builder_get_object(builder, "btCancel"));
    g_signal_connect(dlg->btCancel, "clicked", G_CALLBACK(onCancel), dlg);

    dlg->edAddr = (GtkEntry*)gtk_builder_get_object(builder, "edAddr");
    dlg->edUser = (GtkEntry*)gtk_builder_get_object(builder, "edUser");
    dlg->edPassw = (GtkEntry*)gtk_builder_get_object(builder, "edPassw");
    dlg->edTestAddr = (GtkEntry*)gtk_builder_get_object(builder, "edTestAddr");
    gtk_widget_set_tooltip_text (dlg->edTestAddr, "Checks port 80");  // port 80 will be checked.

    g_object_unref(G_OBJECT (builder));
    
    // dlg = &(dlg->window)
    return dlg;
}


void configDialogClose(ConfigDialog* dlg)
{
    gtk_widget_destroy(GTK_WIDGET(dlg->window));
    g_free(dlg);
}


void configDialogShow(ConfigDialog* dlg)
{
    gtk_window_move(GTK_WINDOW(dlg->window), dlg->rect.x, dlg->rect.y);
    
    ConfigData* data = &dlg->data;

    gtk_entry_set_text(dlg->edAddr, data->servAddr);
    gtk_entry_set_text(dlg->edUser, data->user);
    // after show passw is empty
    if (data->passw)
        gtk_entry_set_text(dlg->edPassw, data->passw);
    gtk_entry_set_text(dlg->edTestAddr, data->testAddr);

    gtk_widget_show_all(dlg->window);
}
