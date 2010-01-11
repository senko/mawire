#ifndef _UTIL_H_
#define _UTIL_H_

#include <glib.h>

#ifndef NDEBUG
#define DEBUG g_debug
#else
#define DEBUG(x...)
#endif

#define MAEMOPAEDIA_GCONF_DB_FNAME "/apps/maemopaedia/database"

gboolean launch_browser (const gchar *url);
gchar *uncompress_string (gpointer data, gint len);
gchar *get_dbname_from_gconf (void);
void save_dbname_to_gconf (gchar *fname);

#endif
