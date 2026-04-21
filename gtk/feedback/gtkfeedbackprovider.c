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

#include "gtkdebug.h"
#include "gtkprivate.h"
#include "gtkmodulesprivate.h"

#include "gtkfeedbackprovider-private.h"

typedef struct {
  GtkWidget* widget;
} GtkFeedbackProviderPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GtkFeedbackProvider, gtk_feedback_provider, G_TYPE_OBJECT)

enum
{
  PROP_WIDGET = 1,
  N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { 0, };

static void
gtk_feedback_provider_dispose (GObject *object)
{
  GtkFeedbackProvider *self = GTK_FEEDBACK_PROVIDER (object);
  GtkFeedbackProviderPrivate *priv = gtk_feedback_provider_get_instance_private (self);
  g_clear_weak_pointer (&priv->widget);
  G_OBJECT_CLASS (gtk_feedback_provider_parent_class)->dispose (object);
}

static void
gtk_feedback_provider_get_property (GObject *object, guint prop_id, GValue *val, GParamSpec *pspec)
{
  GtkFeedbackProvider *self = GTK_FEEDBACK_PROVIDER (object);
  GtkFeedbackProviderPrivate *priv = gtk_feedback_provider_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_value_set_object (val, priv->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_feedback_provider_set_property (GObject *object, guint prop_id, const GValue *val, GParamSpec *pspec)
{
  GtkFeedbackProvider *self = GTK_FEEDBACK_PROVIDER (object);
  GtkFeedbackProviderPrivate *priv = gtk_feedback_provider_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_return_if_fail (priv->widget == NULL);
      priv->widget = g_value_get_object (val);
      g_object_add_weak_pointer (G_OBJECT(priv->widget), (gpointer*)&priv->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_feedback_provider_class_init (GtkFeedbackProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  klass->feedback = NULL;
  object_class->dispose = gtk_feedback_provider_dispose;
  object_class->get_property = gtk_feedback_provider_get_property;
  object_class->set_property = gtk_feedback_provider_set_property;

  obj_properties[PROP_WIDGET] =
    g_param_spec_object ("widget", NULL, NULL,
                         GTK_TYPE_WIDGET,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

static void
gtk_feedback_provider_init (GtkFeedbackProvider *self)
{
  GtkFeedbackProviderPrivate *priv = gtk_feedback_provider_get_instance_private (self);
  priv->widget = NULL;
}

GtkFeedbackProvider *
gtk_feedback_provider_new (GtkWidget *widget)
{
  GIOExtensionPoint *ep;
  GList *l;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  ep = g_io_extension_point_lookup (GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME);
  l = g_io_extension_point_get_extensions (ep);

  if (l)
    return g_object_new (g_io_extension_get_type (l->data), "widget", widget, NULL);
  else
    return g_object_new (GTK_TYPE_FEEDBACK_PROVIDER_NOOP, "widget", widget, NULL);
}

GtkWidget *
gtk_feedback_provider_get_widget (GtkFeedbackProvider *self)
{
  GtkFeedbackProviderPrivate *priv;
  g_return_val_if_fail (GTK_IS_FEEDBACK_PROVIDER (self), FALSE);
  priv = gtk_feedback_provider_get_instance_private (self);
  return priv->widget;
}

gboolean
gtk_feedback_provider_feedback (GtkFeedbackProvider *self, GdkDevice *dev, GtkFeedback feedback)
{
  GtkFeedbackProviderClass *klass;

  g_return_val_if_fail (GTK_IS_FEEDBACK_PROVIDER (self), FALSE);
  if (!gtk_feedback_provider_get_widget (self))
    return FALSE;

  klass = GTK_FEEDBACK_PROVIDER_GET_CLASS (self);
  g_return_val_if_fail (klass->feedback, FALSE);

  return klass->feedback (self, dev, feedback);
}

#ifdef GDK_WINDOWING_ANDROID
extern GType gtk_feedback_provider_android_get_type (void);
#endif

void
gtk_feedback_provider_extension_init (void)
{
  GIOExtensionPoint *ep;
  GIOModuleScope *scope;
  gchar **paths;

  GTK_DEBUG (MODULES, "Registering extension point %s", GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME);

  ep = g_io_extension_point_register (GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME);
  g_io_extension_point_set_required_type (ep, GTK_TYPE_FEEDBACK_PROVIDER);

#ifdef GDK_WINDOWING_ANDROID
  g_type_ensure (gtk_feedback_provider_android_get_type ());
#endif

  scope = g_io_module_scope_new (G_IO_MODULE_SCOPE_BLOCK_DUPLICATES);

  paths = _gtk_get_module_path ("feedback");
  for (int i = 0; paths[i]; i++)
    {
      GTK_DEBUG (MODULES, "Scanning io modules in %s", paths[i]);
      g_io_modules_scan_all_in_directory_with_scope (paths[i], scope);
    }
  g_strfreev (paths);

  g_io_module_scope_free (scope);
}
