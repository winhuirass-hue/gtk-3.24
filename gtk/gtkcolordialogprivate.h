/* GTK - The GIMP Toolkit
 * Copyright (C) 2026 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gtkcolordialog.h"
#include "gdk/gdkcolorprivate.h"

G_BEGIN_DECLS

void            gtk_color_dialog_choose_color           (GtkColorDialog       *self,
                                                         GtkWindow            *parent,
                                                         const GdkColor       *initial_color,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);

GdkColor *      gtk_color_dialog_choose_color_finish    (GtkColorDialog       *self,
                                                         GAsyncResult         *result,
                                                         GError              **error);

G_DECLARE_INTERFACE (GtkColorSelection, gtk_color_selection, GTK, COLOR_SELECTION, GtkWindow)

struct _GtkColorSelectionInterface
{
  GTypeInterface base_interface;

  void (* setup)     (GtkColorSelection *selection,
                      GtkWindow         *parent,
                      const GdkColor    *initial,
                      GtkColorDialog    *self);

  void (* get_color) (GtkColorSelection *selection,
                      GdkColor          *color);

  gpointer padding[12];
};

/**
 * GTK_COLOR_SELECTION_EXTENSION_POINT_NAME:
 *
 * The default extension point name for color selection.
 */
#define GTK_COLOR_SELECTION_EXTENSION_POINT_NAME "gtk-color-selection"

#define GTK_TYPE_COLOR_SELECTION (gtk_color_selection_get_type ())

GIOExtension * gtk_color_selection_get_extension  (void);
void           gtk_color_selection_extension_init (void);

G_END_DECLS
