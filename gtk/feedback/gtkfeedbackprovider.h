/*
 * Copyright (c) 2026 Florian "sp1rit" <sp1rit@disroot.org>
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
 * Authors: Florian Leander Singer <sp1rit@disroot.org>
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/feedback/gtkfeedback.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

/**
 * GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME:
 *
 * The default extension point name for media file.
 */
#define GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME "gtk-feedback-provider"

#define GTK_TYPE_FEEDBACK_PROVIDER (gtk_feedback_provider_get_type ())

GDK_AVAILABLE_IN_4_24
G_DECLARE_DERIVABLE_TYPE (GtkFeedbackProvider, gtk_feedback_provider, GTK, FEEDBACK_PROVIDER, GObject)

struct _GtkFeedbackProviderClass
{
  GObjectClass parent_class;

  gboolean (* feedback) (GtkFeedbackProvider *self, GdkDevice *dev, GtkFeedback feedback);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GDK_AVAILABLE_IN_4_24
GtkWidget *gtk_feedback_provider_get_widget (GtkFeedbackProvider *self);

GDK_AVAILABLE_IN_4_24
gboolean   gtk_feedback_provider_feedback   (GtkFeedbackProvider *self, GdkDevice *dev, GtkFeedback feedback);

G_END_DECLS

