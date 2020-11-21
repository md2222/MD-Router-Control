#ifndef __PREFDIALOG_H__
#define __PREFDIALOG_H__

#include <gtk/gtk.h>
#include <string>
#include "main.h"

using namespace std;

#define MAX_PATH 260


class PrefDialog
{
public:
    GtkWidget  *window;
    string* servAddr = NULL;
    string* user = NULL;
    gchar* passw = NULL;
    string* testAddr = NULL;
    PrefDialog(GtkWidget *parent, const char* uiDir);
    ~PrefDialog();
    void setRect(Rect* r);
    Rect getRect();
    void show();
    //void hide();

private:
    GError *error = NULL;
    gchar* uiPath;
    GtkWidget *btOk;
    GtkWidget *btCancel;
    GtkEntry* edAddr;
    GtkEntry* edUser;
    GtkEntry* edPassw;
    GtkEntry* edTestAddr;
    Rect rect;
    void createDialog();
    static void onOkCb(GtkWidget *widget, PrefDialog* win) {  win->onOk();  };
    static void onCancelCb(GtkWidget *widget, PrefDialog* win) {  win->onCancel();  };
    static gboolean onConfigureCb(GtkWindow *window, GdkEvent *event, PrefDialog* win)  { return win->onConfigure(event);  };
    static gboolean onCloseCb(GtkWindow *window, GdkEvent *event, PrefDialog* win)  {  return win->onClose();  };
    gboolean onConfigure(GdkEvent *event);
    void onOk();
    void onCancel();
    gboolean onClose();
    void procPassw(bool save);
};


#endif
