#include <gtk/gtk.h>
#include <hildon/hildon.h>

#include "util.h"
#include "db.h"
#include "ui.h"

static void
settings_cb (GtkWidget *widget, GtkWidget *window)
{
  gchar *db_fname = show_filename_chooser (window, DEFAULT_DATABASE_FOLDER);

  if (db_fname)
    {
      db_open (db_fname);
      save_dbname_to_gconf (db_fname);
    }

  g_free (db_fname);
}

static void
about_cb (GtkWidget *widget, GtkWidget *window)
{
  // show_about_dialog ();
}

static void
selected_cb (gchar *title)
{
  gchar *text = db_fetch_article (title);

  if (text)
      show_article_window (title, text);
}

static void
search_cb (GtkWidget *widget, GtkWidget *window)
{
  gchar *txt = get_query_string (window);

  if (!txt)
      return;

  show_results_window (txt, db_search (txt),
      G_CALLBACK(selected_cb));
}

static void
random_cb (GtkWidget *widget, GtkWidget *window)
{
}

int
main (int argc, char **argv)
{
  HildonProgram *program;
  GtkWidget *window;
  gchar *db_fname;

  hildon_gtk_init (&argc, &argv);

  g_set_application_name ("Maemopaedia");

  program = hildon_program_get_instance ();

  window = show_main_window (G_CALLBACK (settings_cb),
      G_CALLBACK (about_cb), G_CALLBACK (search_cb),
      G_CALLBACK (random_cb));

  hildon_program_add_window (program, HILDON_WINDOW (window));

  db_fname = get_dbname_from_gconf ();
  if (!db_fname)
    {
      db_fname = show_filename_chooser (window, DEFAULT_DATABASE_FOLDER);

      /* TODO: show a nice error message here before exiting. */
      if (!db_fname)
          return 0;
    }

  db_open (db_fname);
  g_free (db_fname);

  gtk_main ();
  return 0;
}

