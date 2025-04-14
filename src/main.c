#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libsecret/secret.h>

#include <sys/socket.h>
#include <netdb.h>  // h_errno

#include "webwin.h"
#include "conffile.h"
#include "confdialog.h"


gchar* appName = "MD Router Control";
GdkPixbuf *appIcon = 0;
GtkWindow *webWin = 0;
GdkRectangle webWinRect = { 200, 100, 1280, 900 };

gchar* confPath = 0;
GKeyFile* confFile = 0;

GtkStatusIcon *tray = 0;
GdkPixbuf *pixConnected = 0;
GdkPixbuf *pixDisconnected = 0;

ConfigData confData = { 0, 0, 0, 0 };
ConfigDialog* confWin = 0;
GdkRectangle confWinRect = { 0, 0, 0, 0 };


// free() after use
char* currTimeStr()
{
    //char sz[32];
    int bufLen = 32;
    char* sz = malloc(bufLen);
    time_t tt;
    struct tm lt;
    time (&tt);
    localtime_r(&tt, &lt);
    strftime(sz, bufLen, "%Y-%m-%d %H:%M:%S", &lt);
    return sz;
}

//----------------------------------------------------------------------------------------------------------------------
// https://developer.gnome.org/gtk3/stable/GtkDialog.html#GtkDialogFlags

gint MessageBox(GtkWidget *parent, const char* text, const char* caption, uint type, GdkRectangle* rect)
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

/*gint MessageBox(GtkWidget *parent, const char* text, const char* caption, uint type)
{
    return MessageBox(GtkWidget *parent, const char* text, const char* caption, uint type, NULL);
}*/

gboolean hideMessage (gpointer data)
{
    GtkWidget *widget = (GtkWidget *)data;
    gtk_widget_destroy(widget);

    return FALSE;
}


void showMessage (const char* text, int to)
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
    
    int timeout = to >= 5 ? to : 5;
    g_timeout_add (timeout * G_TIME_SPAN_MILLISECOND, hideMessage, (gpointer)window);  
}
//----------------------------------------------------------------------------------------------------------------------


static void saveSettings(ConfigData* data)
{
    confFile = confOpen(confPath);

    if (!confFile)
        fprintf(stderr, "Key file was not opened.\n");
    else
    {
        confSetRect(confFile, "private", "webWinRect", &webWinRect);
        
        confSetRect(confFile, "private", "confWinRect", &confWinRect);

        if (data)
        {
            confSetString(confFile, "public", "servAddr", data->servAddr);
            confSetString(confFile, "public", "user", data->user);
            confSetString(confFile, "public", "testAddr", data->testAddr);
        }

        confSaveToFile(confFile, confPath);
    }
}


static void loadSettings(ConfigData* data)
{
    confFile = confOpen(confPath);

    if (!confFile)
        fprintf(stderr, "conf.open error\n");
    else
    {
        GdkRectangle rect = confGetRect(confFile, "private", "webWinRect");
        if (!confRectIsEmpty(&rect))  webWinRect = rect;

        rect = confGetRect(confFile, "private", "confWinRect");
        if (!confRectIsEmpty(&rect))  confWinRect = rect;

        if (data)
        {
            data->servAddr = confGetString(confFile, "public", "servAddr", "");
            data->user = confGetString(confFile, "public", "user", "");
            data->testAddr = confGetString(confFile, "public", "testAddr", "");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

const SecretSchema mdrctrl_secret_schema = {
	"org.mdrctrl.passw",
	SECRET_SCHEMA_DONT_MATCH_NAME,
	{
		{ "device", SECRET_SCHEMA_ATTRIBUTE_STRING },
		//{ "simid", SECRET_SCHEMA_ATTRIBUTE_STRING },
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
    else 
    {
        //passw = &password;
        //secret_password_free(password);
    }  
    
    return password;      
}

//----------------------------------------------------------------------------------------------------------------------

#define ICON_DISC 0
#define ICON_CONN 1

void setTrayIcon(int id, const char* tooltip)
{
    static int status = 0;
    gchar* currTooltip = 0;
    gboolean isConnected = id == ICON_CONN;

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


gboolean httpPing(const char* addr)
{
    if (!addr || !strlen(addr))  return FALSE;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Open socket error: %d: %s\n", errno, strerror(errno));
        return FALSE;
    }
    
    struct hostent *serv = gethostbyname(addr);
    if (serv == NULL)
    {
        fprintf(stderr, "gethostbyname error: %d: %s\n", h_errno, hstrerror(h_errno));
        // EADDRNOTAVAIL	99	/* Cannot assign requested address */
        // EHOSTDOWN	112	/* Host is down */
        // EFAULT		14	/* Bad address */
        errno = 99;
        return FALSE;
    }

    struct sockaddr_in saddr;
    bzero((char *) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    bcopy((char *)serv->h_addr, (char *)&saddr.sin_addr.s_addr, serv->h_length);
    //saddr.sin_addr.s_addr = inet_addr(addr);
    saddr.sin_port = htons(80);

    int flags = fcntl(sock, F_GETFL, NULL);
    fcntl(sock, F_SETFL, flags|O_NONBLOCK);

    gboolean ok = FALSE;

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

// httpPingLater mode
#define PM_NONE 0
#define PM_MENU 1

void httpPingLater(int mode)
{
    g_print("httpPingLater:   mode=%d    %s\n", mode, confData.testAddr);
    static gboolean active = FALSE;
    if (active)  return FALSE;
    active = TRUE;

    gboolean ok = FALSE;
    gchar* tooltip;

    // ok icon by default
    if (!confData.testAddr || !strlen(confData.testAddr))
    {
        ok = TRUE;
        tooltip = g_strconcat(appName, "\nStatus unknown\nTest address is empty", NULL);
    }
    else
    {
        ok = httpPing(confData.testAddr);

        if (ok)
            tooltip = g_strconcat(appName, "\nConnected", NULL);
        else
            tooltip = g_strconcat(appName, "\nNot connected\n", strerror(errno), NULL);
    }

    int iconId = ok ? ICON_CONN : ICON_DISC;
    setTrayIcon(iconId, tooltip);
    
    if (mode == PM_MENU)
        showMessage(tooltip, 5);

    active = FALSE;
}
//----------------------------------------------------------------------------------------------------------------------

int pingCountdown = 0;


static gint onPingTime(gpointer data)
{
    if (--pingCountdown <= 0)
    {
        httpPingLater(PM_NONE);
        return FALSE;  // stop timer
    }
    
    return TRUE;
}


static void setPingCountdown(int count)
{
    if (pingCountdown <= 0)
        g_timeout_add(1000, onPingTime, NULL);

    pingCountdown = count;
}


static void onNetworkChanged(GNetworkMonitor *monitor, gboolean available, gpointer data)
{
    setPingCountdown(5);
}
//----------------------------------------------------------------------------------------------------------------------


static gboolean onWebWinClose(GtkWidget *win, GdkEventKey* event, gpointer data)
{
    gtk_window_get_position(win, &webWinRect.x, &webWinRect.y);
    gtk_window_get_size(win, &webWinRect.width, &webWinRect.height);

    webWin = 0;
    
    setPingCountdown(3);

    return FALSE;
}


static void onWebWinOpen()
{
    if (webWin)  return;

    webWin = webWindowCreate();

    if (appIcon)
        gtk_window_set_icon (webWin, appIcon);
    
    g_signal_connect(webWin, "delete-event", G_CALLBACK(onWebWinClose), webWin);

    if (!confRectIsEmpty(&webWinRect))
        webWindowSetRect(webWin, &webWinRect); 

    webWindowShow(webWin);
    
    
    gchar* passw = findPassw();
    if (!passw)
    {
        MessageBox(NULL, "Couldn't find password in the secret service.", appName, 0, &confWinRect);
        return;  // !
    }

    gchar *url = g_strconcat("http://", confData.user, ":", passw, "@", confData.servAddr, NULL);

    secret_password_free(passw);

    webWindowLoadUri(webWin, url);

    secret_password_free(url);
}


static gboolean onConfWinClose(GtkWidget *win, GdkEvent *ev, ConfigDialog* dlg)
{
    if (dlg->lastResult)
    {
        confData = dlg->data;
        saveSettings(&confData);
    }
    
    confWinRect = dlg->rect;

    confWin = 0;

    return FALSE;
}

// example
void onConfWinOk()
{
    //g_print("onConfWinOk:   \n");
    ;
}


static void onConfWin()
{
    if (confWin)  return;
    
    confWin = configDialogCreate();
    if (!confWin)  
    {
        g_print("Config dialog create error.\n");
        return;
    }
    
    // The user-provided signal handlers are called in the order they were connected in.
    g_signal_connect(confWin->window, "delete-event", G_CALLBACK(onConfWinClose), confWin);

    confWin->data = confData;
    confWin->data.passw = findPassw();
    confWin->onOk = onConfWinOk;  // can works too

    if (!confRectIsEmpty(&confWinRect))
        confWin->rect = confWinRect;

    configDialogShow(confWin);

    if (confWin->data.passw)
        secret_password_free(confWin->data.passw);
}


static void onTestConn()
{
    g_idle_add(httpPingLater, PM_MENU);
}


static void 
onExit(GtkWidget *menuItem, GApplication *app)
{
    if (webWin)
    {
        gtk_window_close(webWin);
    }
    
    saveSettings(NULL);

    GNetworkMonitor  *netMon = g_network_monitor_get_default ();
    g_signal_handlers_disconnect_by_func(netMon, (void*)onNetworkChanged, NULL);

    gtk_main_quit();
}


static GtkWidget*
trayMenuNew (GtkApplication *app)
{
    GtkWidget *trayMenu = gtk_menu_new();

    GtkWidget *miRouter = gtk_menu_item_new_with_label("Router");
    gtk_widget_show(miRouter);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miRouter);
    g_signal_connect(miRouter, "activate", G_CALLBACK(onWebWinOpen), NULL);

    GtkWidget *miTestConn = gtk_menu_item_new_with_label("Test connection");
    gtk_widget_show(miTestConn);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miTestConn);
    g_signal_connect(miTestConn, "activate", G_CALLBACK(onTestConn), NULL);

    GtkWidget *miOptions = gtk_menu_item_new_with_label("Options");
    gtk_widget_show(miOptions);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miOptions);
    g_signal_connect(miOptions, "activate", G_CALLBACK(onConfWin), NULL);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), sep);

    GtkWidget *miExit = gtk_menu_item_new_with_label("Quit MD Router Control");
    gtk_widget_show(miExit);
    gtk_menu_shell_append(GTK_MENU_SHELL(trayMenu), miExit);
    g_signal_connect(miExit, "activate", G_CALLBACK(onExit), app);

    return trayMenu;
}


static void onTrayActivate(GtkStatusIcon *status_icon, gpointer user_data)
{
    static gboolean active = FALSE;
    static time_t prev_tt = 0;

    time_t tt;
    time(&tt);

    if (tt - prev_tt <= 0)  return;
    prev_tt = tt;

    if (active)  return;
    active = TRUE;

    if (webWin)
    {
        gtk_window_close(webWin);
    }
    else
        onWebWinOpen();

    active = FALSE;
}



static void trayIconMenu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer popupMenu)
{
    gtk_menu_popup(GTK_MENU(popupMenu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
}




typedef struct 
{
    int argc;
    char **argv;
} Args;


static void
appActivate (GtkApplication *app, Args* args)
{
    g_print("MD Router Control 2.1.1      12.04.2025\n");

    gchar *baseName = g_path_get_basename(args->argv[0]);
    gchar *configDir = g_get_user_config_dir();
    confPath = g_strconcat(configDir, "/", baseName, ".conf", NULL);
    g_print("confPath=%s\n", confPath);

    g_free(baseName);
    g_free(configDir);

    g_autoptr(GError) err = NULL;
    appIcon = gdk_pixbuf_new_from_resource ("/app/icons/mdrctrl.png", &err);
    if (!appIcon && err)
        g_warning ("Load window icon error: %s\n", err->message);
        
    pixConnected = gdk_pixbuf_new_from_resource("/app/icons/trayIcon-2.png", NULL);
    pixDisconnected  = gdk_pixbuf_new_from_resource("/app/icons/trayIcon-1.png", NULL);

    tray = gtk_status_icon_new_from_pixbuf(pixDisconnected);

    GtkWidget *trayMenu = trayMenuNew(app);

    g_signal_connect(tray, "popup-menu", G_CALLBACK(trayIconMenu), trayMenu);
    g_signal_connect(tray, "activate", G_CALLBACK(onTrayActivate), app);

    gtk_status_icon_set_visible(tray, TRUE);
    gtk_status_icon_set_tooltip_text(tray, appName);
    
    loadSettings(&confData);

    GNetworkMonitor *netMon = g_network_monitor_get_default();
    g_signal_connect(netMon, "network-changed", G_CALLBACK(onNetworkChanged), NULL);

    g_idle_add(httpPingLater, PM_NONE);

     gtk_main ();
}


int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    Args args;
    args.argc = argc;
    args.argv = argv;
    
    app = gtk_application_new ("org.gtk.mdrctrl", G_APPLICATION_NON_UNIQUE);
    g_signal_connect (app, "activate", G_CALLBACK (appActivate), &args);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}

