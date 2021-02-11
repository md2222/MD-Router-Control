#ifndef __MAIN_H__
#define __MAIN_H__

#include <gtk/gtk.h>
#include <string>
//#include "log.h"

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


void savePassw(const gchar* passw);
bool httpPing(const char* addr);
gboolean httpPingLater(gpointer data);
void saveSettings();


#endif
