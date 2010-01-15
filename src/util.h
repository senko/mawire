#ifndef _UTIL_H_
#define _UTIL_H_

#include <glib.h>

#ifndef NDEBUG
#define DEBUG g_debug
#else
#define DEBUG(x...)
#endif

#define MAWIRE_GCONF_DB_FNAME "/apps/mawire/database"

void gconf_wrapper_init (void);
void gconf_wrapper_dispose (void);

gboolean launch_browser (const gchar *url);
gchar *uncompress_string (gpointer data, gint len);
gchar *get_dbname_from_gconf (void);
void save_dbname_to_gconf (gchar *fname);
gboolean keyboard_is_open (void);

void set_keyboard_slide_callback (GFunc cb, gpointer user_data);

#endif
