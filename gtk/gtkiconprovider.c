/*
 * Copyright © 2024 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "gtkiconproviderprivate.h"
#include "gtkicontheme.h"
#include "gtkenums.h"
#include "gtkwidgetprivate.h"
#include "gtkprivate.h"


/**
 * GtkIconProvider:
 *
 * Interface to override icon lookup.
 *
 * The GtkIconProvider interface is used to look up icons by name.
 * GTK has a builtin icon provider, but platform libraries can
 * override it to implement their own icon lookup policies.
 *
 * Since: 4.22
 */

G_DEFINE_INTERFACE (GtkIconProvider, gtk_icon_provider, G_TYPE_OBJECT)

enum {
  INVALIDATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
gtk_icon_provider_default_init (GtkIconProviderInterface *iface)
{
  /**
   * GtkIconProvider::invalidate:
   * @provider: an icon provider
   *
   * Emitted when the icons need to be looked up again.
   */

  signals[INVALIDATE] =
    g_signal_new ("invalidate",
                  GTK_TYPE_ICON_PROVIDER,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

typedef struct
{
  GdkDisplay *display;
  GtkIconProvider *provider;
  int serial;
} IconProviderData;

static void
invalidated (GtkIconProvider *provider,
             gpointer         data)
{
  IconProviderData *d = data;

  d->serial++;
  gtk_system_setting_changed (d->display, GTK_SYSTEM_SETTING_ICON_THEME);
}

static void
icon_provider_data_free (gpointer data)
{
  IconProviderData *d = data;

  if (d->provider)
    {
      g_signal_handlers_disconnect_by_func (d->provider, invalidated, data);
      g_object_unref (d->provider);
    }

  g_free (d);
}

/**
 * gtk_icon_provider_set_for_display:
 * @display: the display
 * @provider: the icon provider to use
 *
 * Sets the icon provider to use for icon lookups on this display.
 *
 * This API is expected to be used by platform libraries.
 *
 * Since: 4.22
 */
void
gtk_icon_provider_set_for_display (GdkDisplay      *display,
                                   GtkIconProvider *provider)
{
  IconProviderData *data;

  data = g_object_get_data (G_OBJECT (display), "--gtk-icon-provider");
  if (!data)
    {
      data = g_new0 (IconProviderData, 1);
      data->display = display;
      g_object_set_data_full (G_OBJECT (display), "--gtk-icon-provider",
                              data, icon_provider_data_free);
    }

  if (data->provider)
    g_signal_handlers_disconnect_by_func (data->provider, invalidated, data);

  g_set_object (&data->provider, provider);

  if (data->provider)
    g_signal_connect (data->provider, "invalidate",
                      G_CALLBACK (invalidated), data);

  invalidated (data->provider, data);
}

/**
 * gtk_icon_provider_get_for_display:
 * @display: the display
 *
 * Returns the icon provider that has been set with
 * [id@gtk_icon_provider_set_for_display], if any.
 *
 * Note that GTK currently installs the default [class@Gtk.IconTheme]
 * as icon provider.
 *
 * Returns: (nullable): the icon provider
 *
 * Since: 4.22
 */
GtkIconProvider *
gtk_icon_provider_get_for_display (GdkDisplay *display)
{
  IconProviderData *data;

  data = g_object_get_data (G_OBJECT (display), "--gtk-icon-provider");

  if (data)
    return data->provider;

  return NULL;
}

int
gtk_icon_provider_get_serial (GdkDisplay *display)
{
  IconProviderData *data;

  data = g_object_get_data (G_OBJECT (display), "--gtk-icon-provider");

  if (data)
    return data->serial;

  return 0;
}

/**
 * gtk_icon_provider_invalidate:
 * @provider: the icon provider
 *
 * Called by implementations of `GtkIconProvider` to
 * invalidate their icons.
 *
 * This function will emit the [signal@Gtk.IconProvider.invalidate]
 * signal.
 */
void
gtk_icon_provider_invalidate (GtkIconProvider *provider)
{
  g_return_if_fail (GTK_IS_ICON_PROVIDER (provider));

  g_signal_emit (provider, signals[INVALIDATE], 0);
}

static GdkPaintable *
gtk_lookup_builtin_icon (GdkDisplay       *display,
                         const char       *name,
                         int               size,
                         float             scale,
                         GtkTextDirection  direction)
{
  GdkPaintable *icon = NULL;
  const char *symbolic;
  char *uri = NULL;
  GFile *file;

  if (g_str_has_suffix (name, "-symbolic"))
    symbolic = "";
  else
    symbolic = "-symbolic";

  if (direction == GTK_TEXT_DIR_RTL)
    {
      char *path;

      path = g_strconcat ("/org/gtk/libgtk/icons/", name, symbolic, "-rtl", ".svg", NULL);
      if (g_resources_get_info (path, 0, NULL, NULL, NULL))
        uri = g_strconcat ("resource://", path, NULL);

      g_free (path);
    }

  if (uri == NULL)
    uri = g_strconcat ("resource://", "/org/gtk/libgtk/icons/", name, symbolic, ".svg", NULL);

  file = g_file_new_for_uri (uri);

  if (g_file_query_exists (file, NULL))
    {
      GTK_DISPLAY_DEBUG (display, ICONTHEME,
                         "Looking up builtin icon %s: %s",
                         name, uri);

      icon = GDK_PAINTABLE (gtk_icon_paintable_new_for_file (file, size, (int) ceil (scale)));
    }
  else
    {
      GTK_DISPLAY_DEBUG (display, ICONTHEME,
                         "Looking up builtin icon %s: not found",
                         name);

      icon = GDK_PAINTABLE (gdk_texture_new_from_resource ("/org/gtk/libgtk/icons/16x16/status/image-missing.png"));
    }

  g_object_unref (file);
  g_free (uri);

  return icon;
}

static gboolean
gtk_has_builtin_icon (GdkDisplay *display,
                      const char *name)
{
  const char *symbolic;
  char *path;
  gboolean ret;

  if (g_str_has_suffix (name, "-symbolic"))
    symbolic = "";
  else
    symbolic = "-symbolic";

  path = g_strconcat ("/org/gtk/libgtk/icons/", name, symbolic, ".svg", NULL);
  ret = g_resources_get_info (path, 0, NULL, NULL, NULL);
  g_free (path);

  return ret;
}


/**
 * gtk_lookup_icon:
 * @display: the display to use
 * @name: the icon name
 * @size: the size to display the icon at
 * @scale: the scale to display the icon at
 * @direction: the text direction
 * @flags: flags
 *
 * Looks up an icons using the installed [iface@Gtk.IconProvider]
 * of the display.
 *
 * This function is guaranteed to always return a paintable.
 * If the the provider does not have an icon by this name,
 * GTK will consult its builtin icons and ultimatively
 * fall back to a 'missing image' icon.
 *
 * Note that most icons are scalable, so the @size and
 * @scale will not affect the result in most cases.
 *
 * Since: 4.22
 */
GdkPaintable *
gtk_lookup_icon (GdkDisplay         *display,
                 const char         *name,
                 int                 size,
                 float               scale,
                 GtkTextDirection    direction,
                 GtkIconLookupFlags  flags)
{
  GtkIconProvider *provider;
  GdkPaintable *icon = NULL;

  provider = gtk_icon_provider_get_for_display (display);

  if (provider)
    icon = GTK_ICON_PROVIDER_GET_IFACE (provider)->lookup_icon (provider, name, size, scale, direction, flags);

  if (!icon)
    {
      GTK_DISPLAY_DEBUG (display, ICONTHEME,
                         "%s: Looking up icon %s (size %d@%f): not found",
                         G_OBJECT_TYPE_NAME (provider), name, size, scale);

      icon = gtk_lookup_builtin_icon (display, name, size, scale, direction);
    }

  return icon;
}

gboolean
gtk_has_icon (GdkDisplay *display,
              const char *name)
{
  GtkIconProvider *provider;

  provider = gtk_icon_provider_get_for_display (display);

  if (provider)
    {
      if (GTK_ICON_PROVIDER_GET_IFACE (provider)->has_icon (provider, name))
        return TRUE;
    }

  return gtk_has_builtin_icon (display, name);
}
