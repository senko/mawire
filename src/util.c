#include "util.h"

#include <dbus/dbus-glib.h>
#include <gconf/gconf-client.h>
#include <zlib.h>

static struct {
    GConfClient *gc;
    guint notify_id;
    GFunc cb;
    gpointer cb_user_data;
} gconf_wrapper = { NULL, 0, NULL, NULL };

#define GCONF_SLIDE_OPEN_DIR "/system/osso/af"
#define GCONF_SLIDE_OPEN_BASE "slide-open"
#define GCONF_SLIDE_OPEN (GCONF_SLIDE_OPEN_DIR "/" GCONF_SLIDE_OPEN_BASE)

/* function copied from marnanel's raeddit */
gboolean
launch_browser (const gchar *url)
{
  DBusGConnection *connection;
  GError *error = NULL;

  DBusGProxy *proxy;

  DEBUG ("Launching browser for: %s", url);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION,
                               &error);
  if (connection == NULL)
    {
      g_warning ("%s: error getting D-Bus connection: %s",
          G_STRFUNC, error->message);
      g_error_free (error);
      return FALSE;
    }

  proxy = dbus_g_proxy_new_for_name (connection,
            "com.nokia.osso_browser",
            "/com/nokia/osso_browser/request",
            "com.nokia.osso_browser");

  error = NULL;
  if (!dbus_g_proxy_call (proxy, "load_url", &error,
       G_TYPE_STRING, url,
       G_TYPE_INVALID,
       G_TYPE_INVALID))
    {
      g_error_free (error);
      g_warning ("%s: error launching browser: %s",
          G_STRFUNC, error->message);
      return FALSE;
    }

  return TRUE;
}

gchar *
uncompress_string (gpointer data, gint len)
{
  gchar *buf = NULL;
  gint bufsize = len * 2;
  gulong retlen;

  for (;;)
    {
      gint ret;

      buf = g_malloc (bufsize);
      retlen = bufsize;
      ret = uncompress ((unsigned char *) buf, &retlen, data, len);

      switch (ret)
        {
          case Z_OK:
            buf[retlen] = '\0';
            return buf;

          /* retry with a larger buffer */
          case Z_BUF_ERROR:
            g_free (buf);
            bufsize *= 2;
            break;

          /* unknown error, bail out */
          default:
            g_warning ("%s: unknown zlib error: %d", G_STRFUNC, ret);
            g_free (buf);
            return NULL;
        }
    }

  g_assert_not_reached ();
}

void
gconf_wrapper_init (void)
{
  g_assert (gconf_wrapper.gc == NULL);

  gconf_wrapper.gc = gconf_client_get_default ();
}

void
gconf_wrapper_dispose (void)
{
  g_assert (gconf_wrapper.gc);

  g_object_unref (gconf_wrapper.gc);
  gconf_wrapper.gc = NULL;
}

gchar
*get_dbname_from_gconf (void)
{
  g_assert (gconf_wrapper.gc);
  return gconf_client_get_string (gconf_wrapper.gc, MAWIRE_GCONF_DB_FNAME,
      NULL);
}

void
save_dbname_to_gconf (gchar *fname)
{
  g_assert (gconf_wrapper.gc);
  gconf_client_set_string (gconf_wrapper.gc, MAWIRE_GCONF_DB_FNAME, fname,
      NULL);
}

gboolean
keyboard_is_open (void)
{
  return gconf_client_get_bool (gconf_wrapper.gc, GCONF_SLIDE_OPEN, NULL);
}

static void
gconf_cb (GConfClient *gc, guint cnxn_id, GConfEntry *e, gpointer user_data)
{
  gboolean open;

  open = keyboard_is_open ();
  DEBUG ("Keyboard slide %s", open ? "open" : "closed");

  if (!gconf_wrapper.cb)
      return;

  gconf_wrapper.cb (GINT_TO_POINTER (open), gconf_wrapper.cb_user_data);
}

void
set_keyboard_slide_callback (GFunc cb, gpointer user_data)
{
  g_assert (gconf_wrapper.gc);

  if (gconf_wrapper.notify_id != 0)
    {
      gconf_client_remove_dir (gconf_wrapper.gc,
          GCONF_SLIDE_OPEN_DIR, NULL);
      gconf_client_notify_remove (gconf_wrapper.gc,
          gconf_wrapper.notify_id);

      gconf_wrapper.notify_id = 0;
      gconf_wrapper.cb = NULL;
      gconf_wrapper.cb_user_data = NULL;
    }

  if (cb)
    {
      GError *error = NULL;

      gconf_wrapper.cb = cb;
      gconf_wrapper.cb_user_data = user_data;

      gconf_client_add_dir (gconf_wrapper.gc,
          GCONF_SLIDE_OPEN_DIR, GCONF_CLIENT_PRELOAD_NONE, &error);

      if (error)
        {
          g_warning ("error installing keyboard slide callback: %s",
              error->message);
          g_error_free (error);
          return;
        }

      gconf_wrapper.notify_id = gconf_client_notify_add (gconf_wrapper.gc,
          GCONF_SLIDE_OPEN, gconf_cb, NULL, NULL, &error);

      if (error)
        {
          g_warning ("error installing keyboard slide callback: %s",
              error->message);
          g_error_free (error);
          gconf_client_remove_dir (gconf_wrapper.gc,
              GCONF_SLIDE_OPEN_DIR, NULL);
          return;
        }
    }
}

