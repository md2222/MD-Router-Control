// g++ `pkg-config --cflags gtk+-3.0` -o mdrctrl main.cpp `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gnome-keyring-1` -I/usr/include/webkitgtk-4.0 -I/usr/include/libsoup-2.4
// The code is not exemplary. I know.
#include <stdio.h>
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <webkit2/webkit2.h>
// sudo apt-get install libgnome-keyring-dev
#include <gnome-keyring-1/gnome-keyring.h>
#include <gnome-keyring-1/gnome-keyring-memory.h>
#include <string>
#include <iostream>
#include <fstream>

//#include <sys/statvfs.h>
//#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <libgen.h>


using namespace std;


string appDir;
string confPath;

GtkStatusIcon *tray;

string servAddr;
string user;
string passw;
string testAddr;


struct Rect
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    void clear() { x = y = w = h = 0; };
    bool isEmpty() {  return w * h <= 0;  };
};


struct Point
{
    int x = 0;
    int y = 0;
};


bool httpPing(const char* addr);
gboolean httpPingLater(gpointer data);

//----------------------------------------------------------------------------------------------------------------------
// https://developer.gnome.org/gtk3/stable/GtkDialog.html#GtkDialogFlags

gint MessageBox(GtkWidget *parent, const char* text, const char* caption, uint type = 0)
{
   GtkWidget *dialog ;

   if (type & GTK_BUTTONS_YES_NO)
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, text);
   else
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, text);


   gtk_window_set_title(GTK_WINDOW(dialog), caption);
   gint result = gtk_dialog_run(GTK_DIALOG(dialog));
   gtk_widget_destroy( GTK_WIDGET(dialog) );

   return result;
}
//----------------------------------------------------------------------------------------------------------------------

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


WebView* winRouter = 0;


void WebView::setIcon(const char* fileName)
{
    g_autoptr(GError) err = NULL;

    winIcon = gdk_pixbuf_new_from_file(fileName, &err);

    if (!winIcon)
        g_warning ("Load window icon error: %s\n%s\n", err->message, fileName);
}


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
    gtk_window_set_icon (winWeb, winIcon);
    g_signal_connect(winWeb, "delete-event", G_CALLBACK(onCloseCb), this);
    g_signal_connect_after(winWeb, "key_press_event", G_CALLBACK(onKeyPressCb), this);

    web = WEBKIT_WEB_VIEW(webkit_web_view_new());

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

static void onStorePasswCb(GnomeKeyringResult res, gpointer user_data)
{
    if (res == GNOME_KEYRING_RESULT_OK)
        g_print("Password saved\n");
    else
        g_print("Save password error: %s", gnome_keyring_result_to_message(res));
}


static void savePassw(const gchar* passw)
{
    gnome_keyring_store_password (GNOME_KEYRING_NETWORK_PASSWORD, /* The password type */
                                  GNOME_KEYRING_DEFAULT,          /* Where to save it */
                                  "Password wallet",       /* Password description, displayed to user */
                                  passw,                 /* The password itself */
                                  onStorePasswCb,                /* A function called when complete */
                                  NULL, NULL,                     /* User data for callback, and destroy notify */
                                  "user", "MD Router Control",
                                  "server", "gnome.org",
                                  NULL);
}


static void onFoundPasswCb(GnomeKeyringResult res, const gchar* password, gpointer user_data)
{
    /* user_data will be the same as was passed to gnome_keyring_find_password() */
    if (res == GNOME_KEYRING_RESULT_OK)
    {
        //g_print ("Password found: %s\n", password);
        g_print ("Password found\n");
        passw = password;
    }
    else
        g_print("Couldn't find password: %s", gnome_keyring_result_to_message(res));

    /* Once this function returns |password| will be freed */
}


static void findPassw()
{
    gnome_keyring_find_password (GNOME_KEYRING_NETWORK_PASSWORD,  /* The password type */
                                 onFoundPasswCb,                  /* A function called when complete */
                                 NULL, NULL,                      /* User data for callback, and destroy notify */
                                 "user", "MD Router Control",
                                 "server", "gnome.org",
                                 NULL);
}

//----------------------------------------------------------------------------------------------------------------------

Rect prefDlgRect;

class PrefDialog
{
public:
    GtkWidget  *window;
    string* servAddr;
    string* user;
    string* passw;
    string* testAddr;
    PrefDialog(GtkWidget *parent, const char* uiDir)
    {
        rect.x = 500;  rect.y = 300;  rect.w = 0;  rect.h = 0;

        GtkBuilder *builder = gtk_builder_new();

        char path[256] = { 0 };
        strcat(path, uiDir);
        strcat(path, "options.ui");

        if (gtk_builder_add_from_file(builder, path, &error) == 0)
        {
            g_printerr("Error loading file: %s\n", error->message);
            g_clear_error(&error);
            return;
        }

        window = GTK_WIDGET (gtk_builder_get_object(builder, "dlgOptions"));
        gtk_window_set_title(GTK_WINDOW (window), "Options");
        g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(&onCancelCb), this);  //++
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
    };
    ~PrefDialog()
    {
        gtk_widget_destroy(GTK_WIDGET(window));
    };
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
};


void PrefDialog::setRect(Rect* r)
{
    if (r->x * r->y > 0)
    {
        gtk_window_move(GTK_WINDOW(window), r->x, r->y);
        rect = *r;
    }
}


Rect PrefDialog::getRect()
{
    return rect;
}


void PrefDialog::show()
{
    gtk_entry_set_text(edAddr, servAddr->data());
    gtk_entry_set_text(edUser, user->data());
    gtk_entry_set_text(edPassw, passw->data());
    gtk_entry_set_text(edTestAddr, testAddr->data());

    gtk_widget_show_all(window);
}


void PrefDialog::hide()
{
    gtk_widget_hide(window);
}


void PrefDialog::onConfigure(GdkEvent *event)
{
    rect.x = event->configure.x;
    rect.y = event->configure.y;
}


void PrefDialog::onOk()
{
    const gchar *text = gtk_entry_get_text(edAddr);
    if (text)  servAddr->assign(text);

    text = gtk_entry_get_text(edUser);
    if (text)  user->assign(text);

    text = gtk_entry_get_text(edPassw);
    if (text && passw->compare(text))
    {
        passw->assign(text);
        savePassw(text);
    }

    text = gtk_entry_get_text(edTestAddr);
    if (text)  testAddr->assign(text);

    gtk_widget_hide(window);
}


void PrefDialog::onCancel()
{
    gtk_widget_hide(window);
}

//----------------------------------------------------------------------------------------------------------------------
// http://www.gtkbook.com/gtkbook/keyfile.html
// https://developer.gnome.org/glib/stable/glib-File-Utilities.html

class IniFile
{
public:
    IniFile() {};
    ~IniFile() {};
    g_autoptr(GError) error = NULL;
    bool openFromFile(const char* fileName);
    bool saveToFile();
    void setValue(const char* group, const char* name, const char* val);
    gchar* getValue(const char* group, const char* name, const char* def);
    void setList(const char* group, const char* name, int type, void* list, gsize size);
    void* getList(const char* group, const char* name, int type, gsize size);

private:
    g_autoptr(GKeyFile) file = g_key_file_new ();
    string name;

};


bool IniFile::openFromFile(const char* fileName)
{
    file = g_key_file_new ();

    if (!g_key_file_load_from_file(file, fileName, G_KEY_FILE_NONE, &error))
    {
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading key file: %s", error->message);
        //return false;
    }

    name = fileName;
    return true;
}


bool IniFile::saveToFile()
{
    if (file)
    {
        if (!g_file_test(name.data(), G_FILE_TEST_EXISTS))
        {
            ofstream f(name.data());
            f << "[private]" << std::endl;
            f.close();
            printf("Config file created:  %s\n", name.data());
        }

        error = NULL;

        if (!g_key_file_save_to_file(file, name.data(), &error))
        {
            g_warning ("Error saving key file: %s\n%s\n", error->message, name.data());
            return false;
        }
        file = 0;
    }
}


gchar* IniFile::getValue(const char* group, const char* name, const char* def)
{
    g_autoptr(GError) err = NULL;

    gchar *val = g_key_file_get_string (file, group, name, &err);

    if (val == NULL || g_error_matches(err, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
    {
        error = err;
        g_warning ("Error finding key in key file: %s", error->message);
        val = g_strdup(def);
    }

    return val;
}


void IniFile::setValue(const char* group, const char* name, const char* val)
{
    g_key_file_set_string (file, group, name, val);
}


void IniFile::setList(const char* group, const char* name, int type, void* list, gsize size)
{
    if (type == 1)
    {
        g_key_file_set_integer_list(file, group, name, (gint*)list, size);
    }
}


void* IniFile::getList(const char* group, const char* name, int type, gsize size)
{
    gsize sz = 0;
    void* list = NULL;

    if (type == 1)
    {
        list = g_key_file_get_integer_list(file, group, name, &sz, &error);
    }

    if (sz && sz == size)
        return list;
    else
        return NULL;
}
//----------------------------------------------------------------------------------------------------------------------

PrefDialog* pref = 0;


static void onRouter()
{
    if (winRouter->isActive())
        winRouter->show();
    else
    {
        string url = "http://" + user + ":" + passw + "@" + servAddr;
        //printf("onRouter:   url=%s\n", url.data());
        winRouter->loadUrl(url.data());
    }
}


static void onOptions()
{
    if (!pref)
    {
        pref = new PrefDialog(NULL, appDir.data());
        pref->setRect(&prefDlgRect);
        pref->servAddr = &servAddr;
        pref->user = &user;
        pref->passw = &passw;
        pref->testAddr = &testAddr;
    }

    pref->show();
}


static void onExit()
{
    IniFile ini;
    if (!ini.openFromFile(confPath.data()))
        printf("ini.open error\n");
    else
    {
        Rect r = winRouter->getRect();
        if (!r.isEmpty())
            ini.setList("private", "routerWinRect", 1, &r, 4);

        if (pref)
        {
            r = pref->getRect();
            if (r.x)
                ini.setList("private", "prefWinRect", 1, &r, 4);
        }

        ini.setValue("public", "servAddr", servAddr.data());
        ini.setValue("public", "user", user.data());
        ini.setValue("public", "testAddr", testAddr.data());

        ini.saveToFile();
    }

    gtk_main_quit();
}


gboolean httpPingLater(gpointer data)
{
    if (!testAddr.empty())
    {
        if (httpPing(testAddr.data()))
        {
            printf("httpPing true\n");
            gtk_status_icon_set_from_file(tray, (appDir + "icons/trayIcon-2.png").data());
        }
        else
        {
            printf("httpPing false\n");
            gtk_status_icon_set_from_file(tray, (appDir + "icons/trayIcon-1.png").data());
        }
    }

    return FALSE;
}


static void trayIconMenu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer popUpMenu)
{
    gtk_menu_popup(GTK_MENU(popUpMenu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
}


static void onTrayActivate(GtkStatusIcon *status_icon, gpointer user_data)
{
    static bool wait = false;
    if (wait)  return;
    wait = true;

    if (winRouter->isActive())
    {
        winRouter->close();
    }
    else
        onRouter();

    wait = false;
}


bool httpPing(const char* addr)
{
    if (!addr)  return false;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Open socket error\n");
        return false;
    }

    /*struct hostent *serv = gethostbyname(addr);
    if (serv == NULL)
    {
        fprintf(stderr,"gethostbyname error. No such host\n");
        return false;
    }*/

    struct sockaddr_in saddr;
    bzero((char *) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    //bcopy((char *)serv->h_addr, (char *)&saddr.sin_addr.s_addr, serv->h_length);
    saddr.sin_addr.s_addr = inet_addr(addr);
    saddr.sin_port = htons(80);

    int flags = fcntl(sock, F_GETFL, NULL);
    fcntl(sock, F_SETFL, flags|O_NONBLOCK);

    bool ok = false;

    int res = connect(sock, (struct sockaddr *)&saddr, sizeof(saddr));
    printf("Socket connect result: %d, errno=%d\n", res, errno);

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    // EINTR            4      /* Interrupted system call */
    int n = 10;
    while (n-- >= 0)
    {
        res = select(sock + 1, NULL, &fdset, NULL, &tv);
        printf("Socket select result: %d, errno=%d\n", res, errno);
        if (!(res == -1 && errno == EINTR))
            break;
    }

    if (res == 1)
    {
        int so_error;
        socklen_t len = sizeof so_error;

        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        printf("Socket so_error: %d\n", so_error);

        // ECONNREFUSED	111	/* Connection refused */
        // ENETUNREACH	101	/* Network is unreachable */
        if (so_error == 0 || so_error == ECONNREFUSED)
            ok = true;
    }

    fcntl(sock, F_SETFL, flags);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    return ok;
}


// http://zetcode.com/gui/gtk2/menusandtoolbars/

int main(int argc, char **argv)
{
    printf("MD Router Control 0.0.2        16.11.2020\n");

    int bufSize = 256;
    char buf[256] = { 0 };

    if (readlink("/proc/self/exe", buf, bufSize) != -1)
    {
        dirname(buf);
        strcat(buf, "/");
        //printf("appDir=%s\n", buf);
        //printf("argv[0]=%s\n", argv[0]);
        appDir = buf;
    }

    //gchar *currDir = g_get_current_dir();  // bad when run from desktop
    //char *currDir = get_current_dir_name();
    //printf("currDir=%s\n", currDir);
    //appDir = currDir;

    //const gchar *configDir = g_get_user_config_dir();
    //printf("configDir=%s\n", configDir);
    // g_get_home_dir ()

    printf("appDir=%s\n", appDir.data());

    gchar *baseName = g_path_get_basename(argv[0]);
    //printf("baseName=%s\n", baseName);

    confPath = appDir + baseName + ".conf";
    printf("confPath=%s\n", confPath.data());

    gtk_init (&argc, &argv);

    GtkWidget *trayMenu = gtk_menu_new();

    GtkWidget *miRouter = gtk_menu_item_new_with_label("Router");
    gtk_widget_show(miRouter);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miRouter);
    g_signal_connect(miRouter, "activate", G_CALLBACK(onRouter), NULL);

    GtkWidget *miOptions = gtk_menu_item_new_with_label("Options");
    gtk_widget_show(miOptions);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miOptions);
    g_signal_connect(miOptions, "activate", G_CALLBACK(onOptions), NULL);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), sep);

    GtkWidget *miExit = gtk_menu_item_new_with_label("Quit");
    gtk_widget_show(miExit);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miExit);
    g_signal_connect(miExit, "activate", G_CALLBACK(onExit), NULL);

    tray = gtk_status_icon_new_from_file((appDir + "icons/trayIcon-1.png").data());

    g_signal_connect(tray, "popup-menu", G_CALLBACK(trayIconMenu), trayMenu);
    g_signal_connect(tray, "activate", G_CALLBACK(onTrayActivate), tray);

    gtk_status_icon_set_visible(tray, TRUE);
    gtk_status_icon_set_tooltip_text(tray, "MD Router Control");

    winRouter = new WebView();

    string iconPath = appDir + "icons/" + baseName + ".png";
    winRouter->setIcon(iconPath.data());


    prefDlgRect.clear();

    IniFile ini;
    if (!ini.openFromFile(confPath.data()))
        printf("ini.open error\n");
    else
    {
        Rect r;

        int* list = (int*)ini.getList("private", "routerWinRect", 1, 4);
        if (list)
        {
            r.clear();
            //printf("list = %d  %d  %d  %d\n", list[0], list[1], list[2], list[3]);
            r.x = list[0]; r.y = list[1]; r.w = list[2]; r.h = list[3];
            g_free(list);
            winRouter->setRect(&r);
        }

        list = (int*)ini.getList("private", "prefWinRect", 1, 4);
        if (list)
        {
            prefDlgRect.x = list[0]; prefDlgRect.y = list[1]; prefDlgRect.w = list[2]; prefDlgRect.h = list[3];
            g_free(list);
        }

        servAddr = ini.getValue("public", "servAddr", "");
        user = ini.getValue("public", "user", "");
        testAddr = ini.getValue("public", "testAddr", "");
    }


    findPassw();

    g_idle_add(httpPingLater, NULL);

    gtk_main ();

    return 0;
}
