#include "settings.h"



GKeyFile* iniOpen(const char* fileName)
{
    GKeyFile* file = g_key_file_new ();

    g_autoptr(GError) error = NULL;

    if (!g_key_file_load_from_file(file, fileName, G_KEY_FILE_NONE, &error))
    {
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("Error loading key file: %s", error->message);
    }

    return file;
}


bool iniSaveToFile(GKeyFile* file, const char* fileName)
{
    if (!file)  return false;

    if (!g_file_test(fileName, G_FILE_TEST_EXISTS))
    {
        ofstream f(fileName);
        f << "[private]" << std::endl;
        f.close();
        printf("Config file created:  %s\n", fileName);
    }

    g_autoptr(GError) error = NULL;

    if (!g_key_file_save_to_file(file, fileName, &error))
    {
        g_warning ("Error saving key file: %s\n%s\n", error->message, fileName);
        return false;
    }

    //g_key_file_free(file);

    return true;
}


gchar* iniGetValue(GKeyFile* file, const char* group, const char* name, const char* def)
{
    g_autoptr(GError) err = NULL;

    gchar *val = g_key_file_get_string(file, group, name, &err);

    if (val == NULL || g_error_matches(err, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND))
    {
        g_warning ("Error finding key in key file: %s", err->message);
        val = g_strdup(def);
    }

    return val;
}


void iniSetValue(GKeyFile* file, const char* group, const char* name, const char* val)
{
    g_key_file_set_string (file, group, name, val);
}


void iniSetList(GKeyFile* file, const char* group, const char* name, int type, void* list, gsize size)
{
    if (type == LT_INT)
    {
        g_key_file_set_integer_list(file, group, name, (gint*)list, size);
    }
}


void* iniGetList(GKeyFile* file, const char* group, const char* name, int type, gsize size)
{
    gsize sz = 0;
    void* list = NULL;

    if (type == LT_INT)
    {
        g_autoptr(GError) error = NULL;
        list = g_key_file_get_integer_list(file, group, name, &sz, &error);
    }

    if (sz && sz == size)
        return list;
    else
        return NULL;
}

