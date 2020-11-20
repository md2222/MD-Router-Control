#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <gtk/gtk.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

// http://www.gtkbook.com/gtkbook/keyfile.html
// https://developer.gnome.org/glib/stable/glib-File-Utilities.html

#define LT_INT 1


GKeyFile* iniOpen(const char* fileName);
bool iniSaveToFile(GKeyFile* file, const char* fileName);
gchar* iniGetValue(GKeyFile* file, const char* group, const char* name, const char* def);
void iniSetValue(GKeyFile* file, const char* group, const char* name, const char* val);
void iniSetList(GKeyFile* file, const char* group, const char* name, int type, void* list, gsize size);
void* iniGetList(GKeyFile* file, const char* group, const char* name, int type, gsize size);


#endif
