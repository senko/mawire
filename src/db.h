#ifndef _DB_H_
#define _DB_H_

#include <glib.h>

#define DEFAULT_DATABASE_FILE "/home/user/maemopaedia-huge.db"

void db_close (void);
gboolean db_open (const gchar *fname);
gchar *db_fetch_article (const gchar *title);
GList *db_search (const gchar *query);

#endif
