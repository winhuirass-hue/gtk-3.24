/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2026 Hari Rana <theevilskeleton@riseup.net>
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <gtk/gtkenums.h>

G_BEGIN_DECLS

GDK_AVAILABLE_IN_4_24
GtkDirectionType gtk_keyval_to_physical_direction (guint keyval);

G_END_DECLS
