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

#include "config.h"

#include "gtkfeedbackprovider-private.h"

struct _GtkFeedbackProviderNoop {
  GtkFeedbackProvider parent_instance;
};

G_DEFINE_FINAL_TYPE (GtkFeedbackProviderNoop, gtk_feedback_provider_noop, GTK_TYPE_FEEDBACK_PROVIDER)

static gboolean
gtk_feedback_provider_no_feedback (GtkFeedbackProvider *self, GdkDevice *dev, GtkFeedback feedback)
{
  return FALSE;
}

static void
gtk_feedback_provider_noop_class_init (GtkFeedbackProviderNoopClass *klass)
{
  GTK_FEEDBACK_PROVIDER_CLASS (klass)->feedback = gtk_feedback_provider_no_feedback;
}

static void
gtk_feedback_provider_noop_init (GtkFeedbackProviderNoop *self)
{}
