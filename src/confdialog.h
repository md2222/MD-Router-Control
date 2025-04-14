#ifndef __CONFDIALOG_H__
#define __CONFDIALOG_H__

#include <gtk/gtk.h>


typedef struct
{
    gchar* servAddr;
    gchar* user;
    gchar* passw;
    gchar* testAddr;
} ConfigData;


typedef struct
{
    GtkWindow *window;
    GtkEntry* edAddr;
    GtkEntry* edUser;
    GtkEntry* edPassw;
    GtkEntry* edTestAddr;
    GtkWidget *btCancel;
    GtkWidget *btOk;
    gboolean lastResult;
    void (*onOk)();
    GdkRectangle rect;
    ConfigData data;
} ConfigDialog;


ConfigDialog* configDialogCreate();
void configDialogShow(ConfigDialog* dlg);
void configDialogClose(ConfigDialog* dlg);


#endif
