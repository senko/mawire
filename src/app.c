#include <gtk/gtk.h>
#include <hildon/hildon.h>

#include "util.h"
#include "db.h"
#include "ui.h"

static void
choose_db (GtkWidget *window, gchar *folder)
{
  gchar *db_fname = show_filename_chooser (window, folder);

  if (db_fname)
    {
      db_open (db_fname);
      save_dbname_to_gconf (db_fname);
    }

  g_free (db_fname);
}

static void
installed_db_cb (GtkWidget *widget, GtkWidget *window)
{
  choose_db (window, DEFAULT_DATABASE_FOLDER);
}

static void
custom_db_cb (GtkWidget *widget, GtkWidget *window)
{
  choose_db (window, NULL);
}

static void
about_cb (GtkWidget *widget, GtkWidget *window)
{
  show_about_dialog (window);
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
  gchar *title = NULL;
  gchar *text = NULL;

  title = db_fetch_random_title ();

  if (title)
      text = db_fetch_article (title);

  if (text)
      show_article_window (title, text);

  g_free (title);
  g_free (text);
}

static void
slide_cb (gpointer data, gpointer user_data)
{
  GtkWidget *window = user_data;
  gboolean open = GPOINTER_TO_INT (data);

  set_portrait_mode (window, !open);
}

int
main (int argc, char **argv)
{
  HildonProgram *program;
  GtkWidget *window;
  gchar *db_fname;

  hildon_gtk_init (&argc, &argv);
  gconf_wrapper_init ();

  g_set_application_name ("Mawire");

  program = hildon_program_get_instance ();

  window = show_main_window (G_CALLBACK (installed_db_cb),
      G_CALLBACK (custom_db_cb), G_CALLBACK (about_cb),
      G_CALLBACK (search_cb), G_CALLBACK (random_cb));

  hildon_program_add_window (program, HILDON_WINDOW (window));
  set_portrait_mode (window, !keyboard_is_open ());
  set_keyboard_slide_callback (slide_cb, window);
  gtk_widget_show_all (GTK_WIDGET (window));

  db_fname = get_dbname_from_gconf ();
  if (!db_fname)
    {
      db_fname = show_filename_chooser (window, NULL);

      /* TODO: show a nice error message here before exiting. */
      if (!db_fname)
          return 0;
    }

  db_open (db_fname);
  save_dbname_to_gconf (db_fname);
  g_free (db_fname);

  gtk_main ();

  gconf_wrapper_dispose ();

  return 0;
}

