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

#include "gtkfeedbackprovider.h"

G_BEGIN_DECLS

#define GTK_TYPE_FEEDBACK_PROVIDER_NOOP (gtk_feedback_provider_noop_get_type ())
G_DECLARE_FINAL_TYPE (GtkFeedbackProviderNoop, gtk_feedback_provider_noop, GTK, FEEDBACK_PROVIDER_NOOP, GtkFeedbackProvider)

GtkFeedbackProvider *gtk_feedback_provider_new (GtkWidget *widget);

void gtk_feedback_provider_extension_init (void);

G_END_DECLS

