#ifndef __PREFDIALOG_H__
#define __PREFDIALOG_H__

#include <gtk/gtk.h>
#include <string>
#include "main.h"

using namespace std;


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
    void hide();

private:
    GError *error = NULL;
    GtkWidget *btOk;
    GtkWidget *btCancel;
    GtkEntry* edAddr;
    GtkEntry* edUser;
    GtkEntry* edPassw;
    GtkEntry* edTestAddr;
    Rect rect;
    static void onOkCb(GtkWidget *widget, PrefDialog* win) {  win->onOk();  };
    static void onCancelCb(GtkWidget *widget, PrefDialog* win) {  win->onCancel();  };
    static void onConfigureCb(GtkWindow *window, GdkEvent *event, PrefDialog* win)  {  win->onConfigure(event);  };
    void onConfigure(GdkEvent *event);
    void onOk();
    void onCancel();
    void procPassw(bool save);
};


#endif
