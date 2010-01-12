#include "ui.h"

#include <hildon/hildon.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-button.h>
#include <hildon/hildon-file-chooser-dialog.h>

#include "util.h"

static GtkWidget *
append_menu_button (GtkWidget *menu, const gchar *label)
{
  GtkWidget *btn;

  btn = hildon_gtk_button_new (HILDON_SIZE_AUTO);
  gtk_button_set_label (GTK_BUTTON (btn), label);
  hildon_app_menu_append (HILDON_APP_MENU (menu), GTK_BUTTON (btn));

  return btn;
}

gchar *
show_filename_chooser (GtkWidget *window, gchar *folder)
{
  GtkWidget *dialog;
  gchar *result = NULL;

  dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window),
      GTK_FILE_CHOOSER_ACTION_OPEN);

  if (folder)
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), folder);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }

  gtk_widget_destroy (dialog);

  return result;
}

gchar *
get_query_string (GtkWidget *window)
{
  GtkWidget *entry;
  GtkWidget *content_area;
  gchar *text = NULL;
  GtkWidget *dialog;

  dialog = hildon_dialog_new_with_buttons ("Search articles",
      GTK_WINDOW (window),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_STOCK_FIND, GTK_RESPONSE_ACCEPT, NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  entry = hildon_entry_new (HILDON_SIZE_AUTO);

  gtk_container_add (GTK_CONTAINER (content_area), entry);
  gtk_widget_show_all (content_area);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      text = g_strstrip (g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

      if (text == '\0')
        {
          g_free (text);
          text = NULL;
        }
    }

  gtk_widget_destroy (dialog);

  return text;
}

static GtkWidget *
create_tree_view (GtkListStore **model)
{
  GtkWidget *view;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  view = gtk_tree_view_new ();
  *model = gtk_list_store_new (1, G_TYPE_STRING);

  gtk_tree_view_set_model (GTK_TREE_VIEW (view),
      GTK_TREE_MODEL (*model));

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (col),
      "expand", TRUE,
      NULL);

  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer,
      "text", 0);

  return view;
}

static void
insert_text (GtkTextBuffer *buffer, const gchar *text)
{
  GtkTextMark *emph_point = NULL;
  GtkTextMark *bold_point = NULL;

  while (*text)
    {
      GtkTextIter here;
      gchar *next;

      gtk_text_buffer_get_end_iter (buffer, &here);

      if ((*text == '\'') && (text[1] == '\'') && (text[2] == '\''))
        {
          if (!bold_point)
            {
              bold_point = gtk_text_buffer_create_mark (buffer, "bold",
                  &here, TRUE);
            }
          else
            {
              GtkTextIter past;

              gtk_text_buffer_get_iter_at_mark (buffer, &past, bold_point);
              gtk_text_buffer_apply_tag_by_name (buffer, "bold", &past, &here);
              gtk_text_buffer_delete_mark (buffer, bold_point);
              bold_point = NULL;
            }

          text += 3;
          continue;
        }

      if ((*text == '\'') && (text[1] == '\''))
        {
          if (!emph_point)
            {
              emph_point = gtk_text_buffer_create_mark (buffer, "emph",
                  &here, TRUE);
            }
          else
            {
              GtkTextIter past;

              gtk_text_buffer_get_iter_at_mark (buffer, &past, emph_point);
              gtk_text_buffer_apply_tag_by_name (buffer, "emph", &past, &here);
              gtk_text_buffer_delete_mark (buffer, emph_point);
              emph_point = NULL;
            }

          text += 2;
          continue;
        }

      for (next = g_utf8_next_char (text);
          *next && (*next != '\'');
          next = g_utf8_next_char (next));

      gtk_text_buffer_insert (buffer, &here, text, next - text);

      text = next;
    }
}

static void
open_in_browser (GtkWidget *button, gchar *title)
{
  gchar *valid_chars = "abcdefghijklmnopqrstuvwxyz" \
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
      "0123456789_()[],";
  gchar *q = g_strdup (title);
  gchar *url = g_strdup_printf
    ("http://wikipedia.org/wiki/Special:Search/?search=%s",
     g_strcanon (q, valid_chars, '+'));

  g_free (q);

  launch_browser (url);

  g_free (url);
}

GtkWidget *
show_article_window (gchar *title, gchar *text)
{
  GtkWidget *win;
  GtkWidget *pannable;
  GtkWidget *vbox;
  GtkWidget *title_label;
  GtkWidget *text_box;
  GtkTextBuffer *buffer;
  GtkWidget *more;
  gchar *pango;

  win = hildon_stackable_window_new ();
  gtk_window_set_title (GTK_WINDOW (win), title);

  g_signal_connect (G_OBJECT (win), "delete-event",
      G_CALLBACK (gtk_widget_destroy), win);

  pannable = hildon_pannable_area_new ();

  g_object_set (pannable,
      "mov-mode", HILDON_MOVEMENT_MODE_VERT,
      "hscrollbar-policy", GTK_POLICY_NEVER,
      NULL);

  pango = g_markup_printf_escaped ("<span size='xx-large'>%s</span>", title);
  title_label = gtk_label_new (text);
  gtk_label_set_markup (GTK_LABEL (title_label), pango);
  gtk_misc_set_alignment (GTK_MISC (title_label), 0.0, 0.5);

  text_box = hildon_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_box), GTK_WRAP_WORD_CHAR);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_box));

  gtk_text_buffer_create_tag (buffer, "bold", "weight", PANGO_WEIGHT_BOLD,
      NULL);
  gtk_text_buffer_create_tag (buffer, "emph", "style", PANGO_STYLE_ITALIC,
      NULL);

  insert_text (buffer, text);
  vbox = gtk_vbox_new (FALSE, 10);

  gtk_box_pack_start (GTK_BOX (vbox), title_label, FALSE, FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vbox), text_box, TRUE, TRUE, 0);

  more = hildon_button_new_with_text (
      HILDON_SIZE_FINGER_HEIGHT,
      HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
      "Read complete article on Wikipedia", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), more, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (win), pannable);
  hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (pannable),
      vbox);

  g_signal_connect (G_OBJECT (more), "clicked",
      G_CALLBACK (open_in_browser), g_strdup (title)); /* FIXME: memleak! */

  gtk_widget_show_all (win);
  return win;
}

static GtkWidget *
create_app_menu (GtkWidget *win, GCallback installed_db_cb,
    GCallback custom_db_cb, GCallback about_cb)
{
  GtkWidget *menu;
  GtkWidget *use_installed_btn;
  GtkWidget *use_custom_btn;
  GtkWidget *about_btn;

  menu = hildon_app_menu_new ();

  use_installed_btn = append_menu_button (menu, "Use installed DB");
  use_custom_btn = append_menu_button (menu, "Use custom DB");
  about_btn = append_menu_button (menu, "About");

  g_signal_connect (G_OBJECT (use_installed_btn), "clicked",
      G_CALLBACK (installed_db_cb), win);
  g_signal_connect (G_OBJECT (use_custom_btn), "clicked",
      G_CALLBACK (custom_db_cb), win);
  g_signal_connect (G_OBJECT (about_btn), "clicked",
      G_CALLBACK (about_cb), win);

  gtk_widget_show_all (GTK_WIDGET (menu));

  return menu;
}

static void
row_activated_cb (GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *column, GSourceFunc selected_cb)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *text;

  model = gtk_tree_view_get_model (view);
  if (!gtk_tree_model_get_iter (model, &iter, path))
      return;

  gtk_tree_model_get (model, &iter, 0, &text, -1);
  selected_cb (text);
  g_free (text);
}

GtkWidget *
show_results_window (gchar *query, GList *results,
  GCallback selected_cb)
{
  GtkWidget *win;
  GtkWidget *pannable;
  GtkWidget *view;
  GtkListStore *model;
  GtkWidget *aa;
  GtkWidget *n_results_label;
  GtkTreePath *exact_match = NULL;
  gchar *query_icase;
  gint n_results;
  gchar *tmp;

  query_icase = g_utf8_casefold (query, -1);
  n_results = g_list_length (results);

  DEBUG ("Showing %d results for: %s", n_results, query);

  win = hildon_stackable_window_new ();
  gtk_window_set_title (GTK_WINDOW (win), "Search results");

  g_signal_connect (G_OBJECT (win), "delete-event",
      G_CALLBACK (gtk_widget_destroy), win);

  pannable = hildon_pannable_area_new ();

  g_object_set (pannable,
      "mov-mode", HILDON_MOVEMENT_MODE_VERT,
      "hscrollbar-policy", GTK_POLICY_NEVER,
      NULL);

  view = create_tree_view (&model);

  aa = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (view));
  hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (view), TRUE);

  tmp = g_strdup_printf ("Found %s%d articles about %s",
      (n_results == 500) ? "more than " : "",
      n_results, query);

  n_results_label = gtk_label_new (tmp);
  g_free (tmp);

  gtk_container_add (GTK_CONTAINER (aa), n_results_label);
  gtk_container_add (GTK_CONTAINER (win), pannable);
  gtk_container_add (GTK_CONTAINER (pannable), view);

  g_signal_connect (G_OBJECT (view), "row-activated",
      G_CALLBACK (row_activated_cb), selected_cb);

  while (results)
    {
      GtkTreeIter iter;
      tmp = results->data;
      gchar *tmp_icase = g_utf8_casefold (tmp, -1);

      gtk_list_store_insert_with_values (model, &iter, -1, 0, tmp, -1);

      if (!g_utf8_collate (tmp_icase, query_icase))
          exact_match = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);

      results = g_list_remove (results, tmp);
      g_free (tmp);
      g_free (tmp_icase);
    }

  /* if there's exact match in the results, we want to scroll
   * to it so the user doesn't need to scroll potentially huge
   * list to find it. */
  if (exact_match)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), exact_match,
          NULL, FALSE, 0.0, 0.0);
      gtk_tree_path_free (exact_match);
      exact_match = NULL;
    }

  gtk_widget_show_all (win);
  return win;
}

GtkWidget *
show_main_window (GCallback installed_db_cb, GCallback custom_db_cb,
    GCallback about_cb, GCallback search_clicked_cb,
    GCallback random_clicked_cb)
{
  GtkWidget *window;
  GtkWidget *search_btn;
  GtkWidget *random_btn;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *vbox;
  GtkWidget *hbox;

  window = hildon_stackable_window_new ();

  g_signal_connect (G_OBJECT (window), "delete-event",
      G_CALLBACK (gtk_main_quit), NULL);

  hildon_window_set_app_menu (HILDON_WINDOW (window),
      HILDON_APP_MENU (create_app_menu (window,
          installed_db_cb, custom_db_cb, about_cb)));

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (TRUE, 10);

  label = gtk_label_new ("Maemopaedia, the offline Wikipedia reader");

  image = gtk_image_new_from_file (MAIN_WINDOW_IMAGE);

  gtk_box_pack_start (GTK_BOX (vbox), image, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 5);

  search_btn = hildon_button_new_with_text (
      HILDON_SIZE_FINGER_HEIGHT,
      HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
      "Search", NULL);

  random_btn = hildon_button_new_with_text (
      HILDON_SIZE_FINGER_HEIGHT,
      HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
      "Random", NULL);

  gtk_box_pack_start (GTK_BOX (hbox), search_btn, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), random_btn, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 5);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  g_signal_connect (G_OBJECT (search_btn), "clicked",
      G_CALLBACK (search_clicked_cb), window);

  /* We haven't implemented Random yet, don't confuse the user. */
  gtk_widget_set_sensitive (random_btn, FALSE);
  g_signal_connect (G_OBJECT (random_btn), "clicked",
      G_CALLBACK (random_clicked_cb), window);

  gtk_widget_show_all (GTK_WIDGET (window));
  return window;
}

void
show_about_dialog (GtkWidget *window)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *ca;

  dialog = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
      GTK_WINDOW (window));

  gtk_window_set_title (GTK_WINDOW (dialog), "About Maemopaedia");

  label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_label_set_markup (GTK_LABEL (label),
    "<span size='x-large'>Maemopaedia 0.1</span>\n" \
    "<i>Senko Rasic &lt;senko.rasic@collabora.co.uk&gt;</i>\n\n" \
    "Articles by Wikipedia contributors\n" \
    "Used under Creative Commons Attribution Share-Alike license");

  ca = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_add (GTK_CONTAINER (ca), label);
  gtk_widget_show_all (ca);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}


