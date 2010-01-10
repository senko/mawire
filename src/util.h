#ifndef _UTIL_H_
#define _UTIL_H_

#include <glib.h>

#ifndef NDEBUG
#define DEBUG g_debug
#else
#define DEBUG(x...)
#endif

gboolean launch_browser (const gchar *url);
gchar *uncompress_string (gpointer data, gint len);

#endif
