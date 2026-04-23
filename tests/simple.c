/*
 * Copyright (C) 2017  Red Hat, Inc
 * Author: Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include <gtk/gtk.h>

typedef enum
{
  GTK_EDGE_NONE,
  GTK_EDGE_NW,
  GTK_EDGE_NNW,
  GTK_EDGE_N,
  GTK_EDGE_NNE,
  GTK_EDGE_NE,
  GTK_EDGE_ENE,
  GTK_EDGE_E,
  GTK_EDGE_ESE,
  GTK_EDGE_SE,
  GTK_EDGE_SSE,
  GTK_EDGE_S,
  GTK_EDGE_SSW,
  GTK_EDGE_SW,
  GTK_EDGE_WSW,
  GTK_EDGE_W,
  GTK_EDGE_WNW,
} GtkResizeEdge;

G_DECLARE_FINAL_TYPE (GtkResizeHandle, gtk_resize_handle, GTK, RESIZE_HANDLE, GtkWidget)

struct _GtkResizeHandle
{
  GtkWidget parent_instance;
  GtkResizeEdge edge;
};

struct _GtkResizeHandleClass
{
  GtkWidgetClass parent_class;
};

G_DEFINE_TYPE (GtkResizeHandle, gtk_resize_handle, GTK_TYPE_WIDGET)

static void
gtk_resize_handle_init (GtkResizeHandle *self)
{
}

static void
gtk_resize_handle_class_init (GtkResizeHandleClass *class)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (class), "resize-handle");
}

static GtkWidget *
gtk_resize_handle_new (GtkResizeEdge edge)
{
  struct {
    const char *name;
    const char *cursor;
    gboolean hexpand;
    gboolean vexpand;
  } handles[] = {
    { NULL, NULL },
    { "NW",  "nw-resize" },
    { "NNW", "nw-resize" },
    { "N",   "n-resize", .hexpand=1, },
    { "NNE", "ne-resize" },
    { "NE",  "ne-resize" },
    { "ENE", "ne-resize" },
    { "E",   "e-resize", .vexpand=1, },
    { "ESE", "se-resize" },
    { "SE",  "se-resize" },
    { "SSE", "se-resize" },
    { "S",   "s-resize", .hexpand=1, },
    { "SSW", "sw-resize" },
    { "SW",  "sw-resize" },
    { "WSW", "sw-resize" },
    { "W",   "w-resize", .vexpand=1, },
    { "WNW", "nw-resize" },
  };
  GtkWidget *self = g_object_new (gtk_resize_handle_get_type (), NULL);
  ((GtkResizeHandle *) self)->edge = edge;
  gtk_widget_set_tooltip_text (self, handles[edge].name);
  gtk_widget_set_cursor_from_name (self, handles[edge].cursor);
  gtk_widget_add_css_class (self, handles[edge].name);
  gtk_widget_set_hexpand (self, handles[edge].hexpand);
  gtk_widget_set_vexpand (self, handles[edge].vexpand);
  return self;
}

const char css[] =
"resize-handle {"
"  min-width: 10px;"
"  min-height: 10px;"
"  background-color: teal;"
"}"
"resize-handle.NNW,"
"resize-handle.NNE,"
"resize-handle.SSW,"
"resize-handle.SSE {"
"  min-width: 20px;"
"}"
"resize-handle.WNW,"
"resize-handle.ENE,"
"resize-handle.WSW,"
"resize-handle.ESE {"
"  min-height: 20px;"
"}"
"resize-handle:hover {"
"  background-color: magenta;"
"}";

int
main (int argc, char *argv[])
{
  GtkWidget *window, *grid;
  GtkCssProvider *provider;

  gtk_init ();

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (provider, css);
  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                              GTK_STYLE_PROVIDER (provider),
                                              1000);
  window = gtk_window_new ();

  grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_NW),  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_NNW), 1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_N),   2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_NNE), 3, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_NE),  4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_WNW), 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_ENE), 4, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_W),   0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_E),   4, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_WSW), 0, 3, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_ESE), 4, 3, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_SW),  0, 4, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_SSW), 1, 4, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_S),   2, 4, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_SSE), 3, 4, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_resize_handle_new (GTK_EDGE_SE),  4, 4, 1, 1);
  gtk_window_set_child (GTK_WINDOW (window), grid);

  gtk_window_present (GTK_WINDOW (window));

  while (1)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
