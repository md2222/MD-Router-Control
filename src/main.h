#ifndef __MAIN_H__
#define __MAIN_H__

#include <gtk/gtk.h>
#include <string>

using namespace std;


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


/*
#define LOG_PRFX_NONE 0
#define LOG_PRFX_TIME 1
#define LOG_PRFX_SPACE 2
*/
class Log
{
private:
    FILE* f;
    //QMutex mtx;
public:
    enum { LOG_PRFX_NONE, LOG_PRFX_TIME, LOG_PRFX_SPACE };
    Log(char *path)
    {
        f = fopen(path, "a");
        if (!f)
            fprintf(stderr, "Open log file error: %d: %s\n", errno, strerror(errno));
    }
    ~Log()
    {
        if (f)  fclose(f);
    }
    void write(int mode, const char *format, ...)
    {
        va_list args;
        //string s, msg;
        gchar *s;

        va_start(args, format);
        gchar *msg = g_strdup_vprintf(format, args);
        va_end(args);

        if (mode == LOG_PRFX_SPACE)
            s = g_strconcat("    ", msg, NULL);
        else if (mode == LOG_PRFX_TIME)
        {
            char sz[32];
            time_t tt = time(NULL);
            struct tm lt;
            localtime_r(&tt, &lt);
            strftime(sz, sizeof(sz), "%Y-%m-%d %H:%M:%S    ", &lt);
            s = g_strconcat(sz, msg, NULL);
        }
        else
            s = msg;

        //QMutexLocker locker(&mtx);

        if (!f)
            fprintf(stderr, "%s\n", s);
        else
            if (fwrite(s, 1, strlen(s), f) < 0)
                fprintf(stderr, "Write log file error: %d: %s\n", errno, strerror(errno));

        if (s != msg)  g_free(msg);
        g_free(s);
    }
};


void savePassw(const gchar* passw);
bool httpPing(const char* addr);
gboolean httpPingLater(gpointer data);
void saveSettings();


#endif
