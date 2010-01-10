#include "db.h"

#include <sqlite3.h>
#include <string.h>

#include "util.h"

static sqlite3 *db_handle = NULL;

void
db_close (void)
{
  if (db_handle != NULL)
    {
      sqlite3_close (db_handle);
      db_handle = NULL;
    }
}

gboolean
db_open (const gchar *fname)
{
  gint ret;

  DEBUG ("Opening database: %s", fname);

  db_close ();

  ret = sqlite3_open (fname, &db_handle);

  if (ret == SQLITE_OK)
    {
      return TRUE;
    }
  else
    {
      g_warning ("%s: error opening database: %s",
          G_STRFUNC, sqlite3_errmsg (db_handle));
      db_handle = NULL;
      return FALSE;
    }
}

gchar *
db_fetch_article (const gchar *title)
{
  gint ret;
  sqlite3_stmt *stmt;
  gchar *article = NULL;

  DEBUG ("Fetching article: %s", title);

  if (!db_handle)
      return NULL;

  ret = sqlite3_prepare_v2 (db_handle,
      "SELECT text FROM articles WHERE title = ?", -1, &stmt, NULL);

  if (ret != SQLITE_OK)
    {
      g_warning ("%s: error preparing SQL statement: %s",
          G_STRFUNC, sqlite3_errmsg (db_handle));
      return NULL;
    }

  ret = sqlite3_bind_text (stmt, 1, title, -1, SQLITE_STATIC);

  if (ret != SQLITE_OK)
    {
      g_warning ("%s: error binding to SQL statement: %s",
          G_STRFUNC, sqlite3_errmsg (db_handle));
      sqlite3_finalize (stmt);
      return NULL;
    }

  ret = sqlite3_step (stmt);

  if (ret == SQLITE_ROW)
    {
      gint len = sqlite3_column_bytes (stmt, 0);
      gpointer blob = g_memdup (sqlite3_column_blob (stmt, 0), len);

      article = uncompress_string (blob, len);
      g_free (blob);
    }
  else
    {
      /* no results is ok, but errors we'd like to report */
      if (ret != SQLITE_DONE)
          g_warning ("%s: error fetching article: %s",
              G_STRFUNC, sqlite3_errmsg (db_handle));
    }

  sqlite3_finalize (stmt);
  return article;
}

static GList *
get_results (sqlite3_stmt *stmt)
{
  GList *li = NULL;
  const unsigned char *col;

  for (;;)
    {
      switch (sqlite3_step (stmt))
        {
          case SQLITE_ROW:
            col = sqlite3_column_text (stmt, 0);
            if (col && *col)
                li = g_list_prepend (li, g_strdup ((const gchar *) col));
            break;

          case SQLITE_DONE:
            if (li != NULL)
                li = g_list_sort (li, (GCompareFunc) g_strcmp0);
            return li;

          /* unknown error, we'll just return empty list */
          default:
            g_warning ("%s: error fetching results: %s",
                G_STRFUNC, sqlite3_errmsg (db_handle));
            return NULL;
        }
    }

  g_assert_not_reached ();
}

GList *
db_search (const gchar *query)
{
  gint ret;
  gchar *match;
  sqlite3_stmt *stmt;
  GString *str;
  gchar **tokens;
  int i;
  GList *li = NULL;

  DEBUG ("Searching the database for: %s", query);

  ret = sqlite3_prepare_v2 (db_handle,
      "SELECT content FROM article_index WHERE "
          "content MATCH ? ORDER BY LENGTH(content) ASC LIMIT 500",
      -1, &stmt, NULL);

  if (ret != SQLITE_OK)
    {
      g_warning ("%s: error preparing SQL statement: %s",
          G_STRFUNC, sqlite3_errmsg (db_handle));
      return NULL;
    }

  str = g_string_sized_new (strlen (query) * 2);
  tokens = g_strsplit (query, " ", -1);

  for (i = 0; tokens[i]; i++)
    {
      /* very short search tokens cause massive performance
       * hit, better to ignore them. */
      if (strlen (tokens[i]) > 2)
        {
          g_string_append (str, tokens[i]);
          g_string_append (str, "* ");
        }
    }

  match = g_string_free (str, FALSE);
  if (*match != '\0')
    {
      ret = sqlite3_bind_text (stmt, 1, match, -1, SQLITE_STATIC);

      if (ret == SQLITE_OK)
        {
          li = get_results (stmt);
        }
      else
        {
          g_warning ("%s: error binding to SQL statement: %s",
              G_STRFUNC, sqlite3_errmsg (db_handle));
        }
    }

  sqlite3_finalize (stmt);
  g_free (match);

  return li;
}

