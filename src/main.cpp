// g++ `pkg-config --cflags gtk+-3.0` -o mdrctrl main.cpp `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gnome-keyring-1` -I/usr/include/webkitgtk-4.0 -I/usr/include/libsoup-2.4
// The code is not exemplary. I know.
#include <stdio.h>
#include <gtk/gtk.h>
// sudo apt install libsecret-1-dev
#include <libsecret/secret.h>
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

GtkApplication *app = 0;
char* appTitle = "MD Router Control";
//string appDir;
string appDataDir;
string confPath;

GtkStatusIcon *tray = 0;
GdkPixbuf *pixConnected = 0;
GdkPixbuf *pixDisconnected = 0;
GtkWidget *trayMenu = 0;
bool isMenuAction = false;

string servAddr;
string user;
//string passw;
string testAddr;
bool useSystemThemeForScrollBars = true;

GKeyFile* iniFile;

PrefDialog* pref = 0;
Rect prefDlgRect;
WebView* winRouter = 0;

//Log log("/tmp/mdrctrl.log");

void setTrayIcon(bool isConnected, const char* tooltip);
static void onNetworkChanged(GNetworkMonitor *monitor, gboolean available, gpointer data);


string currTimeStr()
{
    char sz[32];
    time_t tt;
    struct tm lt;
    time (&tt);
    localtime_r(&tt, &lt);
    strftime(sz, sizeof(sz), "%Y-%m-%d %H:%M:%S", &lt);
    return string(sz);
}


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

const SecretSchema mdrctrl_secret_schema = {
	"org.mdrctrl.passw",
	SECRET_SCHEMA_DONT_MATCH_NAME,
	{
		{ "device", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "NULL", (SecretSchemaAttributeType)0 },
	}
};


void savePassw(const gchar* passw)
{
    if (!passw)
        g_print("Password pointer is NULL\n");
    else if (strlen(passw) > 0)
    {
        GError *error = NULL;

        secret_password_store_sync (&mdrctrl_secret_schema, SECRET_COLLECTION_DEFAULT,
                                    "mdrctrl", passw, NULL, &error,
                                    "device", "router",
                                    NULL);

        if (error != NULL) 
        {
            g_print("Save password error: %s\n", error->message);
            g_error_free (error);
        } 
        else 
        {
            g_print("Password saved\n");
        }  
    }
    else
    {
        GError *error = NULL;

        gboolean removed = secret_password_clear_sync (&mdrctrl_secret_schema, NULL, &error,
                                                       "device", "router",
                                                       NULL);

        if (error != NULL) {
            g_print("Delete password error: %s\n", error->message);
            g_error_free (error);
        } 
        /* removed will be TRUE if a password was removed */
        else if (!removed)
        {
            g_print("Password was not deleted for an unknown reason\n");
        } 
        else
            g_print("Password deleted\n");
     
    }
}


gchar* findPassw()
{
    GError *error = NULL;

    gchar *password = secret_password_lookup_sync (&mdrctrl_secret_schema, NULL, &error,
                                                   "device", "router",
                                                   NULL);

    if (error != NULL) 
    {
        g_print("Lookup password error: %s\n", error->message);
        g_error_free (error);

    } 
    else if (password == NULL) 
    {
        /* password will be null, if no matching password found */
        g_print("Couldn't find password in the secret service\n");
    } 

    //secret_password_free(password);
    
    return password;      
}

//----------------------------------------------------------------------------------------------------------------------

bool httpPing(const char* addr)
{
    if (!addr || !strlen(addr))  return false;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Open socket error: %d: %s\n", errno, strerror(errno));
        return false;
    }

    fprintf(stderr, "%s    gethostbyname...\n", currTimeStr().data());
    struct hostent *serv = gethostbyname(addr);
    if (serv == NULL)
    {
        fprintf(stderr, "gethostbyname error: %d: %s\n", h_errno, hstrerror(h_errno));
        // EADDRNOTAVAIL	99	/* Cannot assign requested address */
        // EHOSTDOWN	112	/* Host is down */
        // EFAULT		14	/* Bad address */
        errno = 99;
        return false;
    }
    fprintf(stderr, "%s    gethostbyname end\n", currTimeStr().data());

    struct sockaddr_in saddr;
    bzero((char *) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    bcopy((char *)serv->h_addr, (char *)&saddr.sin_addr.s_addr, serv->h_length);
    //saddr.sin_addr.s_addr = inet_addr(addr);
    saddr.sin_port = htons(80);

    int flags = fcntl(sock, F_GETFL, NULL);
    fcntl(sock, F_SETFL, flags|O_NONBLOCK);

    bool ok = false;

    // ENETUNREACH	101	/* Network is unreachable */
    // EINPROGRESS  115 /* Operation now in progress */
    int res = connect(sock, (struct sockaddr *)&saddr, sizeof(saddr));
    //printf("Socket connect result: %d, errno=%d\n", res, errno);

    if (res < 0 && errno != EINPROGRESS)
    {
        fprintf(stderr, "Socket connect error: %d: %s\n", errno, strerror(errno));
    }
    else
    {
        // EINTR            4      /* Interrupted system call */
        int n = 6;
        while (n-- >= 0)
        {
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);
            struct timeval tv;
            tv.tv_sec = 10;
            tv.tv_usec = 0;

            res = select(sock + 1, NULL, &fdset, NULL, &tv);
            fprintf(stderr, "Socket select result: %d, err: %d %s\n", res, errno, strerror(errno));
            if (res == -1)
            {
                if (errno == EINTR)
                    continue;
                else
                    break;
            }

            int so_error;
            socklen_t len = sizeof so_error;

            // ECONNREFUSED	111	/* Connection refused */ - refused by server?
            // ENETUNREACH	101	/* Network is unreachable */
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
            fprintf(stderr, "Socket so_error: %d: %s\n", so_error, strerror(so_error));

            if (so_error != ECONNREFUSED)
            {
                if (so_error != 0)
                    break;

                int isset = FD_ISSET(sock, &fdset);
                fprintf(stderr, "Socket FD_ISSET: %d\n", isset);
                if (!isset)
                    continue;
            }

            ok = true;
            break;
        }
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
        //gchar* passw = NULL;
        //findPassw(&passw);
        gchar* passw = findPassw();
        if (!passw)
        {
            //printf("onRouter:   Couldn't find password in keyring.\n");
            MessageBox(NULL, "Couldn't find password in the secret service.", appTitle, 0, &prefDlgRect);
            return;  // !
        }

        gchar *url = g_strconcat("http://", user.data(), ":", passw, "@", servAddr.data(), NULL);
        //printf("onRouter:   url=%s\n", url);
        secret_password_free(passw);

        winRouter->loadUrl(url);

        //gnome_keyring_free_password(url);
        secret_password_free(url);
    }
}

/*
static gboolean onButtonRelease(GtkStatusIcon* self, GdkEventButton ev, gpointer user_data)
{
    //printf("button_release\n");
    printf("button_release   %d   %d\n", ev.x, ev.y);

    gint x, y;

    //gtk_widget_get_pointer(GTK_WIDGET(self), &x, &y);
    //printf("gtk_widget_get_pointer:  %d  %d\n", x, y);

    return TRUE;
}
*/

gboolean hideMessage (gpointer data)
{
    GtkWidget *widget = (GtkWidget *)data;
    gtk_widget_destroy(widget);

    return FALSE;
}


void showMessage (const char* text, int timeout = 5)
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_default_size(GTK_WINDOW(window), 150, 50);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_window_set_accept_focus (GTK_WINDOW(window), false);
    gtk_window_set_keep_above (GTK_WINDOW(window), true);

    
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_save (context);  // no border without it
    gtk_style_context_add_class(context, "tooltip");  // need .tooltip in gtk3.css

    GtkWidget *label = gtk_label_new (text);
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);

    gtk_container_add (GTK_CONTAINER (window), label);
    
    gtk_widget_show_all (window);
    
    g_timeout_add (timeout * G_TIME_SPAN_MILLISECOND, hideMessage, (gpointer)window);  
}


static void onTestConn()
{
    isMenuAction = true;
    g_idle_add(httpPingLater, NULL);
}


static void onOptions()
{
    if (!pref)
    {
        pref = new PrefDialog(NULL, appDataDir.data());
        pref->setRect(&prefDlgRect);
        pref->servAddr = &servAddr;
        pref->user = &user;
        //pref->passw = passw;
        pref->testAddr = &testAddr;
    }

    //gchar* passw = NULL;
    //findPassw(&passw);
    gchar* passw = findPassw();
    pref->passw = passw;

    pref->show();

    if (passw)
        secret_password_free(passw);
}


void saveSettings()
{
    //log.write(Log::LOG_PRFX_TIME, "saveSettings\n");

    if (!iniFile)
        fprintf(stderr, "Key file was not opened.\n");
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

        iniSetValue(iniFile, "private", "useSystemThemeForScrollBars", useSystemThemeForScrollBars ? "1" : "0");

        iniSetValue(iniFile, "public", "servAddr", servAddr.data());
        iniSetValue(iniFile, "public", "user", user.data());
        iniSetValue(iniFile, "public", "testAddr", testAddr.data());

        iniSaveToFile(iniFile, confPath.data());
        //log.write(Log::LOG_PRFX_TIME, "saveSettings - end\n");
    }
}


static void onExit()
{
    saveSettings();

    GNetworkMonitor  *netMon = g_network_monitor_get_default ();
    //g_signal_handlers_disconnect_by_func(netMon, G_CALLBACK(onNetworkChanged), NULL);  // ?
    g_signal_handlers_disconnect_by_func(netMon, (void*)onNetworkChanged, NULL);

    gtk_main_quit();
}

/*
static void onTestAddr(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    printf("onTestAddr: \n");

    g_autoptr(GError) err = NULL;
    gboolean ok = g_network_monitor_can_reach_finish((GNetworkMonitor*)source_object, res, &err);
    printf("onTestAddr: %d\n", ok);
}
*/

void setTrayIcon(bool isConnected, const char* tooltip)
{
    static int status = 0;
    static string currTooltip;

    if (!status || (status > 0) != isConnected)
    {
        status = isConnected ? 1 : -1;

        if (isConnected)
        {
            printf("setTrayIcon: Connected\n");
            gtk_status_icon_set_from_pixbuf(tray, pixConnected);
        }
        else
        {
            printf("setTrayIcon: Disconnected\n");
            gtk_status_icon_set_from_pixbuf(tray, pixDisconnected);
        }
    }

    if (currTooltip != tooltip)
    {
        currTooltip = tooltip;
        gtk_status_icon_set_tooltip_text(tray, tooltip);
    }
}


gboolean httpPingLater(gpointer data)
{
    static bool active = FALSE;
    if (active)  return FALSE;
    active = TRUE;

    //printf("httpPingLater:   testAddr=%s\n", testAddr.data());
    bool ok = FALSE;
    string tooltip = appTitle;

    // ok icon by default
    if (testAddr.empty())
    {
        ok = TRUE;
        tooltip.append("\nStatus unknown\nTest address is empty");
    }
    else
    {
        ok = httpPing(testAddr.data());

        if (ok)
            tooltip.append("\nConnected");
        else
            tooltip.append("\nNot connected\n").append(strerror(errno));
    }

    setTrayIcon(ok, tooltip.data());
    
    if (isMenuAction)
        showMessage(tooltip.data(), 5);
    isMenuAction = false;

    active = FALSE;

    return FALSE;
}


static void trayIconMenu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer popUpMenu)
{
    gtk_menu_popup(GTK_MENU(popUpMenu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
}


static void onTrayActivate(GtkStatusIcon *status_icon, gpointer user_data)
{
    static bool active = false;
    static time_t prev_tt = 0;

    time_t tt;
    time(&tt);

    //printf("onTrayActivate:  tt - prev_tt = %ld\n", tt - prev_tt);
    if (tt - prev_tt <= 0)  return;
    prev_tt = tt;

    if (active)  return;
    active = true;

    if (winRouter->isActive())
    {
        winRouter->close();
    }
    else
        onRouter();

    active = false;
}

int pingCountdown = 0;


gint onPingTimeCb(gpointer data)
{
    //printf("onPingTimeCb:  pingCountdown=%d\n", pingCountdown);
    pingCountdown--;
    if (pingCountdown <= 0)
    {
        //httpPing(testAddr.data());
        httpPingLater(NULL);
        return 0;
    }
    return 1;
}


void setPingCountdown()
{
    //printf("setPingCountdown:  pingCountdown=%d\n", pingCountdown);
    if (pingCountdown <= 0)
        g_timeout_add(1000, onPingTimeCb, NULL);

    pingCountdown = 5;
}


static void onNetworkChanged(GNetworkMonitor *monitor, gboolean available, gpointer data)
{
    //printf("onNetworkChanged:  available=%d\n", available);
    //static int status = 0;

    /*if (!status || (status > 0) != available)
    {
        status = available ? 1 : -1;
        httpPing(testAddr.data());
    }*/
    setPingCountdown();
}


struct Args
{
    int argc;
    char **argv;
};


// http://zetcode.com/gui/gtk2/menusandtoolbars/

//int main(int argc, char **argv)
static void onAppInit(GApplication *app, Args* args)
{
    int bufSize = 256;
    char buf[256] = { 0 };

    /*if (readlink("/proc/self/exe", buf, bufSize) != -1)
    {
        dirname(buf);
        strcat(buf, "/");
        //printf("appDir=%s\n", buf);
        //printf("argv[0]=%s\n", argv[0]);
        appDir = buf;
    } */

    gchar *baseName = g_path_get_basename(args->argv[0]);
    appDataDir = string("/usr/local/share/") + baseName + "/";
    const gchar *configDir = g_get_user_config_dir();
    //printf("configDir=%s\n", configDir);
    // g_get_home_dir ()
    // g_get_user_data_dir()  - ~/.local/share

    confPath = string(configDir) + "/" + baseName + ".conf";
    string logPath = string(configDir) + "/" + baseName + ".log";

    //printf("appDir=%s\n", appDir.data());
    printf("appDataDir=%s\n", appDataDir.data());
    printf("confPath=%s\n", confPath.data());

    freopen(logPath.data(), "a", stderr); //++
    fprintf(stderr, "================================================================================\n%s\n", currTimeStr().data());

    gtk_init (&(args->argc), &(args->argv));

    //GtkWidget *trayMenu = gtk_menu_new();
    trayMenu = gtk_menu_new();

    GtkWidget *miRouter = gtk_menu_item_new_with_label("Router");
    gtk_widget_show(miRouter);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miRouter);
    g_signal_connect(miRouter, "activate", G_CALLBACK(onRouter), NULL);

    GtkWidget *miTestConn = gtk_menu_item_new_with_label("Test connection");
    gtk_widget_show(miTestConn);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miTestConn);
    g_signal_connect(miTestConn, "activate", G_CALLBACK(onTestConn), NULL);

    GtkWidget *miOptions = gtk_menu_item_new_with_label("Options");
    gtk_widget_show(miOptions);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miOptions);
    g_signal_connect(miOptions, "activate", G_CALLBACK(onOptions), NULL);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), sep);

    GtkWidget *miExit = gtk_menu_item_new_with_label("Quit MD Router Control");
    gtk_widget_show(miExit);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miExit);
    g_signal_connect(miExit, "activate", G_CALLBACK(onExit), NULL);

    pixConnected = gdk_pixbuf_new_from_resource("/app/icons/trayIcon-2.png", NULL);
    pixDisconnected  = gdk_pixbuf_new_from_resource("/app/icons/trayIcon-1.png", NULL);

    tray = gtk_status_icon_new_from_pixbuf(pixDisconnected);

    g_signal_connect(tray, "popup-menu", G_CALLBACK(trayIconMenu), trayMenu);
    g_signal_connect(tray, "activate", G_CALLBACK(onTrayActivate), tray);
    //g_signal_connect(tray, "button-release-event", G_CALLBACK(onButtonRelease), NULL);

    gtk_status_icon_set_visible(tray, TRUE);
    gtk_status_icon_set_tooltip_text(tray, appTitle);

    winRouter = new WebView();

    prefDlgRect.clear();

    GdkRectangle workarea = {0};
    gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()), &workarea);

    prefDlgRect.x = workarea.width - 400; prefDlgRect.y = workarea.height - 300;


    iniFile = iniOpen(confPath.data());

    if (!iniFile)
        fprintf(stderr, "ini.open error\n");
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

        useSystemThemeForScrollBars = iniGetValue(iniFile, "private", "useSystemThemeForScrollBars", "1") != string("0");

        list = (int*)iniGetList(iniFile, "private", "prefWinRect", LT_INT, 4);
        if (list)
        {
            prefDlgRect.x = list[0]; prefDlgRect.y = list[1]; prefDlgRect.w = list[2]; prefDlgRect.h = list[3];
            g_free(list);
        }

        servAddr = iniGetValue(iniFile, "public", "servAddr", "");
        user = iniGetValue(iniFile, "public", "user", "");
        testAddr = iniGetValue(iniFile, "public", "testAddr", "");

        winRouter->setUseSystemThemeForScrollBars(useSystemThemeForScrollBars);
    }


    GNetworkMonitor *netMon = g_network_monitor_get_default();
    g_signal_connect(netMon, "network-changed", G_CALLBACK(onNetworkChanged), NULL);


    g_idle_add(httpPingLater, NULL);


    gtk_main ();

    //return 0;
}


void onAppShutdown(GApplication *app, gpointer data)
{
    //log.write(Log::LOG_PRFX_TIME, "onAppShutdown\n");
}


void onSysSignal(int sig)
{
    static bool isHandled = false;
    if (isHandled)  return;
    isHandled = true;

    //sig = 1;
    /*if (sig == SIGHUP)
        log.write(Log::LOG_PRFX_TIME, "SIGHUP\n");
    else if (sig == SIGINT)
        log.write(Log::LOG_PRFX_TIME, "SIGINT\n");
    else if (sig == SIGTERM)
        log.write(Log::LOG_PRFX_TIME, "SIGTERM\n");*/

    /* Do something useful here */
    onExit();

    //exit(EXIT_FAILURE);
}

/*
void onQueryEnd(GtkApplication *app, gpointer data)
{
    log.write(Log::LOG_PRFX_TIME, "onQueryEnd\n");

}
*/

int main(int argc, char **argv)
{
    printf("MD Router Control 2.1.1      23.10.2023\n");
    int status;

    try
    {

    if (signal(SIGHUP, onSysSignal) == SIG_ERR)
        fprintf(stderr, "Can't catch SIGHUP\n");
    if (signal(SIGINT, onSysSignal) == SIG_ERR)
        fprintf(stderr, "Can't catch SIGINT\n");
    if (signal(SIGTERM, onSysSignal) == SIG_ERR)
        fprintf(stderr, "Can't catch SIGTERM\n");

    //GtkApplication *app;
    Args args;

    args.argc = argc;
    args.argv = argv;

    app = gtk_application_new("org.gnome.mdroutercontrol", G_APPLICATION_NON_UNIQUE);
    //bool v = TRUE;  // PROP_REGISTER_SESSION = 1 ?
    //gtk_application_set_property(G_APPLICATION(app), PROP_REGISTER_SESSION, &v, NULL);
    g_signal_connect(app, "activate", G_CALLBACK(onAppInit), &args);
    g_signal_connect(app, "shutdown", G_CALLBACK(onAppShutdown), &args);
    //g_signal_connect (app, "query-end", G_CALLBACK(onQueryEnd), NULL);  // signal 'query-end' is invalid for instance ... of type 'GtkApplication'

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref (app);

    }
    catch (std::exception& ex)
    {
       std::cerr << ex.what() << std::endl;
    }
    catch (...)
    {
       std::cerr << "Unknown exception." << std::endl;
    }

    return status;
}
