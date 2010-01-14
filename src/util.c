#include "util.h"

#include <dbus/dbus-glib.h>
#include <gconf/gconf-client.h>
#include <zlib.h>

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

gchar
*get_dbname_from_gconf (void)
{
  GConfClient *gc;
  gchar *fname;

  gc = gconf_client_get_default ();
  fname = gconf_client_get_string (gc, MAWIRE_GCONF_DB_FNAME, NULL);
  g_object_unref (gc);
  return fname;
}

void
save_dbname_to_gconf (gchar *fname)
{
  GConfClient *gc;

  gc = gconf_client_get_default ();
  gconf_client_set_string (gc, MAWIRE_GCONF_DB_FNAME, fname, NULL);
  g_object_unref (gc);
}


