#ifndef _UI_H_
#define _UI_H_

#include <gtk/gtk.h>

#define MAIN_WINDOW_IMAGE "/home/user/maemopaedia.png"

GtkWidget *show_main_window (GCallback settings_cb, GCallback about_cb,
    GCallback search_clicked_cb, GCallback random_clicked_cb);
GtkWidget *show_results_window (gchar *query, GList *results,
  GCallback selected_cb);
GtkWidget *show_article_window (gchar *title, gchar *text);
gchar *get_query_string (GtkWidget *window);
gchar *show_filename_chooser (GtkWidget *window, gchar *folder);

#endif

