#ifndef _DB_H_
#define _DB_H_

#include <glib.h>

#define DEFAULT_DATABASE_FOLDER "/opt/mawire/data"

void db_close (void);
gboolean db_open (const gchar *fname);
gchar *db_fetch_article (const gchar *title);
GList *db_search (const gchar *query);
gchar *db_fetch_random_title (void);

#endif
