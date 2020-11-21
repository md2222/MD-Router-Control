// g++ `pkg-config --cflags gtk+-3.0` -o mdrctrl main.cpp `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gnome-keyring-1` -I/usr/include/webkitgtk-4.0 -I/usr/include/libsoup-2.4
// The code is not exemplary. I know.
#include <stdio.h>
#include <gtk/gtk.h>
// sudo apt-get install libgnome-keyring-dev
#include <gnome-keyring-1/gnome-keyring.h>
#include <gnome-keyring-1/gnome-keyring-memory.h>

//#include <sys/statvfs.h>
//#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <libgen.h>
#include "main.h"
#include "settings.h"
#include "prefdialog.h"
#include "webview.h"


using namespace std;

char* appTitle = "MD Router Control";
string appDir;
string confPath;

GtkStatusIcon *tray;

string servAddr;
string user;
//string passw;
string testAddr;

GKeyFile* iniFile;

PrefDialog* pref = 0;
Rect prefDlgRect;
WebView* winRouter = 0;

//----------------------------------------------------------------------------------------------------------------------
// https://developer.gnome.org/gtk3/stable/GtkDialog.html#GtkDialogFlags

gint MessageBox(GtkWidget *parent, const char* text, const char* caption, uint type, Rect* rect = 0)
{
   GtkWidget *dialog ;

   if (type & GTK_BUTTONS_YES_NO)
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, text);
   else
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, text);


   gtk_window_set_title(GTK_WINDOW(dialog), caption);

   if (rect && rect->x)
       gtk_window_move(GTK_WINDOW(dialog), rect->x, rect->y);

   gint result = gtk_dialog_run(GTK_DIALOG(dialog));

   gtk_widget_destroy( GTK_WIDGET(dialog) );

   return result;
}
//----------------------------------------------------------------------------------------------------------------------

void savePassw(const gchar* passw)
{
    if (!passw)
        g_print("Password pointer is NULL\n");
    else if (strlen(passw) > 0)
    {
        GnomeKeyringResult result = gnome_keyring_store_password_sync(
                    GNOME_KEYRING_NETWORK_PASSWORD,
                    GNOME_KEYRING_DEFAULT,
                    "MD Router Control",
                    passw,
                    "user", "MD Router Control",
                    "server", "gnome.org",
                    NULL);

        if (result == GNOME_KEYRING_RESULT_OK)
            g_print("Password saved\n");
        else
            g_print("Save password error: %s", gnome_keyring_result_to_message(result));
    }
    else
    {
        GnomeKeyringResult result = gnome_keyring_delete_password_sync(
                    GNOME_KEYRING_NETWORK_PASSWORD,
                    "user", "MD Router Control",
                    "server", "gnome.org",
                    NULL);

        if (result == GNOME_KEYRING_RESULT_OK)
            g_print("Password deleted\n");
        else
            g_print("Delete password error: %s", gnome_keyring_result_to_message(result));
    }
}


void findPassw(gchar **passw)
{
    GnomeKeyringResult result = gnome_keyring_find_password_sync(
                GNOME_KEYRING_NETWORK_PASSWORD,
                passw,
                "user", "MD Router Control",
                "server", "gnome.org",
                NULL);

    if (result != GNOME_KEYRING_RESULT_OK)
    {
        g_print("Couldn't find password: %s", gnome_keyring_result_to_message(result));
        //passw = "";
    }
    else
        g_print ("Password found\n");
}

//----------------------------------------------------------------------------------------------------------------------

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
//----------------------------------------------------------------------------------------------------------------------


static void onRouter()
{
    if (winRouter->isActive())
        winRouter->show();
    else
    {
        gchar* passw = NULL;
        findPassw(&passw);
        if (!passw)
        {
            printf("onRouter:   Couldn't find password in keyring.\n");
            MessageBox(NULL, "Couldn't find password in keyring.", appTitle, 0, &prefDlgRect);
            return;
        }

        gchar *url = g_strconcat("http://", user.data(), ":", passw, "@", servAddr.data(), NULL);
        //printf("onRouter:   url=%s\n", url);
        gnome_keyring_free_password(passw);

        winRouter->loadUrl(url);

        gnome_keyring_free_password(url);
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
        //pref->passw = passw;
        pref->testAddr = &testAddr;
    }

    gchar* passw = NULL;
    findPassw(&passw);
    pref->passw = passw;

    pref->show();

    gnome_keyring_free_password(passw);
}


static void onExit()
{
    if (!iniFile)
        printf("Key file was not opened.\n");
    else
    {
        Rect r = winRouter->getRect();
        if (!r.isEmpty())
            iniSetList(iniFile, "private", "routerWinRect", LT_INT, &r, 4);

        if (pref)
        {
            r = pref->getRect();
            if (r.x)
                iniSetList(iniFile, "private", "prefWinRect", LT_INT, &r, 4);
        }

        iniSetValue(iniFile, "public", "servAddr", servAddr.data());
        iniSetValue(iniFile, "public", "user", user.data());
        iniSetValue(iniFile, "public", "testAddr", testAddr.data());

        iniSaveToFile(iniFile, confPath.data());
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


// http://zetcode.com/gui/gtk2/menusandtoolbars/

int main(int argc, char **argv)
{
    printf("MD Router Control 0.0.5        21.11.2020\n");

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

    //const gchar *configDir = g_get_user_config_dir();
    //printf("configDir=%s\n", configDir);
    // g_get_home_dir ()
    // g_get_user_data_dir()  - ~/.local/share

    printf("appDir=%s\n", appDir.data());

    gchar *baseName = g_path_get_basename(argv[0]);

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
    gtk_status_icon_set_tooltip_text(tray, appTitle);

    winRouter = new WebView();

    string iconPath = appDir + "icons/" + baseName + ".png";
    winRouter->setIcon(iconPath.data());


    prefDlgRect.clear();

    GdkRectangle workarea = {0};
    gdk_monitor_get_workarea(
        gdk_display_get_primary_monitor(gdk_display_get_default()),
        &workarea);

    prefDlgRect.x = workarea.width - 400; prefDlgRect.y = workarea.height - 300;


    iniFile = iniOpen(confPath.data());

    if (!iniFile)
        printf("ini.open error\n");
    else
    {
        Rect r;

        int* list = (int*)iniGetList(iniFile, "private", "routerWinRect", LT_INT, 4);
        if (list)
        {
            r.clear();
            //printf("list = %d  %d  %d  %d\n", list[0], list[1], list[2], list[3]);
            r.x = list[0]; r.y = list[1]; r.w = list[2]; r.h = list[3];
            g_free(list);
            winRouter->setRect(&r);
        }

        list = (int*)iniGetList(iniFile, "private", "prefWinRect", LT_INT, 4);
        if (list)
        {
            prefDlgRect.x = list[0]; prefDlgRect.y = list[1]; prefDlgRect.w = list[2]; prefDlgRect.h = list[3];
            g_free(list);
        }

        servAddr = iniGetValue(iniFile, "public", "servAddr", "");
        user = iniGetValue(iniFile, "public", "user", "");
        testAddr = iniGetValue(iniFile, "public", "testAddr", "");
    }


    g_idle_add(httpPingLater, NULL);

    gtk_main ();

    return 0;
}
