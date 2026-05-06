/*
 * Copyright © 2018 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "gtkrootprivate.h"
#include "gtknative.h"
#include "gtknativeprivate.h"
#include "gtkcssnodeprivate.h"
#include "gtkwidgetprivate.h"
#include "gdk/gdkprivate.h"
#include "gtkprivate.h"
#include "gtkwindowprivate.h"

#include "gtkshortcutmanager.h"

/**
 * GtkRoot:
 *
 * An interface for widgets that can act as the root of a widget hierarchy.
 *
 * The root widget takes care of providing the connection to the windowing
 * system and manages layout, drawing and event delivery for its widget
 * hierarchy.
 *
 * The obvious example of a `GtkRoot` is `GtkWindow`.
 *
 * To get the display to which a `GtkRoot` belongs, use
 * [method@Gtk.Root.get_display].
 *
 * `GtkRoot` also maintains the location of keyboard focus inside its widget
 * hierarchy, with [method@Gtk.Root.set_focus] and [method@Gtk.Root.get_focus].
 */

G_DEFINE_INTERFACE_WITH_CODE (GtkRoot, gtk_root, GTK_TYPE_WIDGET,
                              g_type_interface_add_prerequisite (g_define_type_id, GTK_TYPE_NATIVE))

static GdkDisplay *
gtk_root_default_get_display (GtkRoot *self)
{
  return gdk_display_get_default ();
}


static GtkConstraintSolver *
gtk_root_default_get_constraint_solver (GtkRoot *self)
{
  return NULL;
}

static GtkWidget *
gtk_root_default_get_focus (GtkRoot *self)
{
  return NULL;
}

static void
gtk_root_default_set_focus (GtkRoot   *self,
                            GtkWidget *focus)
{
}

static void
gtk_root_default_init (GtkRootInterface *iface)
{
  iface->get_display = gtk_root_default_get_display;
  iface->get_constraint_solver = gtk_root_default_get_constraint_solver;
  iface->get_focus = gtk_root_default_get_focus;
  iface->set_focus = gtk_root_default_set_focus;
}

/**
 * gtk_root_get_display:
 * @self: a `GtkRoot`
 *
 * Returns the display that this `GtkRoot` is on.
 *
 * Returns: (transfer none): the display of @root
 */
GdkDisplay *
gtk_root_get_display (GtkRoot *self)
{
  GtkRootInterface *iface;

  g_return_val_if_fail (GTK_IS_ROOT (self), NULL);

  iface = GTK_ROOT_GET_IFACE (self);
  return iface->get_display (self);
}

GtkConstraintSolver *
gtk_root_get_constraint_solver (GtkRoot *self)
{
  GtkRootInterface *iface;

  g_return_val_if_fail (GTK_IS_ROOT (self), NULL);

  iface = GTK_ROOT_GET_IFACE (self);
  return iface->get_constraint_solver (self);
}

/**
 * gtk_root_set_focus:
 * @self: a `GtkRoot`
 * @focus: (nullable): widget to be the new focus widget, or %NULL
 *    to unset the focus widget
 *
 * If @focus is not the current focus widget, and is focusable, sets
 * it as the focus widget for the root.
 *
 * If @focus is %NULL, unsets the focus widget for the root.
 *
 * To set the focus to a particular widget in the root, it is usually
 * more convenient to use [method@Gtk.Widget.grab_focus] instead of
 * this function.
 */
void
gtk_root_set_focus (GtkRoot   *self,
                    GtkWidget *focus)
{
  g_return_if_fail (GTK_IS_ROOT (self));
  g_return_if_fail (focus == NULL || GTK_IS_WIDGET (focus));

  GTK_ROOT_GET_IFACE (self)->set_focus (self, focus);
}

/**
 * gtk_root_get_focus:
 * @self: a `GtkRoot`
 *
 * Retrieves the current focused widget within the root.
 *
 * Note that this is the widget that would have the focus
 * if the root is active; if the root is not focused then
 * `gtk_widget_has_focus (widget)` will be %FALSE for the
 * widget.
 *
 * Returns: (nullable) (transfer none): the currently focused widget
 */
GtkWidget *
gtk_root_get_focus (GtkRoot *self)
{
  g_return_val_if_fail (GTK_IS_ROOT (self), NULL);

  return GTK_ROOT_GET_IFACE (self)->get_focus (self);
}

void
gtk_root_start_layout (GtkRoot *self)
{
  gtk_native_queue_relayout (GTK_NATIVE (self));
}

void
gtk_root_stop_layout (GtkRoot *self)
{
}

void
gtk_root_queue_restyle (GtkRoot *self)
{
  gtk_root_start_layout (self);
}

typedef struct
{
  GList *pointer_foci;
} Foci;

static void
free_foci (gpointer data)
{
  Foci *f = data;
  g_list_free_full (f->pointer_foci, (GDestroyNotify) gtk_pointer_focus_unref);
  g_free (f);
}

static Foci *
gtk_root_get_foci (GtkRoot *root)
{
  Foci *foci = (Foci *) g_object_get_data (G_OBJECT (root), "gtk-root-foci");

  if (!foci)
    {
      foci = g_new0 (Foci, 1);
      g_object_set_data_full (G_OBJECT (root), "gtk-root-foci", foci, free_foci);
    }

  return foci;
}

GtkPointerFocus *
gtk_root_lookup_pointer_focus (GtkRoot          *root,
                               GdkDevice        *device,
                               GdkEventSequence *sequence)
{
  Foci *f = gtk_root_get_foci (root);

  for (GList *l = f->pointer_foci; l; l = l->next)
    {
      GtkPointerFocus *focus = l->data;
      if (focus->device == device && focus->sequence == sequence)
        return focus;
    }

  return NULL;
}

static void
gtk_root_add_pointer_focus (GtkRoot         *root,
                            GtkPointerFocus *focus)
{
  Foci *f = gtk_root_get_foci (root);

  f->pointer_foci = g_list_prepend (f->pointer_foci, gtk_pointer_focus_ref (focus));
}

static void
gtk_root_remove_pointer_focus (GtkRoot         *root,
                               GtkPointerFocus *focus)
{
  Foci *f = gtk_root_get_foci (root);

  if (g_list_find (f->pointer_foci, focus))
    {
      f->pointer_foci = g_list_remove (f->pointer_foci, focus);
      gtk_pointer_focus_unref (focus);
    }
}

static void
set_widget_active_state (GtkWidget *widget,
                         GtkWidget *topmost,
                         gboolean   active)
{
  GtkWidget *w = widget;

  while (w)
    {
      gtk_widget_set_active_state (w, active);
      if (w == topmost)
        break;
      w = _gtk_widget_get_parent (w);
    }
}

void
gtk_root_update_pointer_focus (GtkRoot          *root,
                               GdkDevice        *device,
                               GdkEventSequence *sequence,
                               GtkWidget        *target,
                               double            x,
                               double            y)
{
  GtkPointerFocus *focus;

  focus = gtk_root_lookup_pointer_focus (root, device, sequence);
  if (focus)
    {
      if (target)
        {
          gtk_pointer_focus_set_target (focus, target);
          gtk_pointer_focus_set_coordinates (focus, x, y);
        }
      else
        {
          if (gtk_pointer_focus_get_implicit_grab (focus))
            {
              set_widget_active_state (focus->grab_widget, NULL, FALSE);
              gtk_pointer_focus_set_implicit_grab (focus, NULL);
            }
          gtk_root_remove_pointer_focus (root, focus);
        }
    }
  else if (target)
    {
      focus = gtk_pointer_focus_new (root, target, device, sequence, x, y);
      gtk_root_add_pointer_focus (root, focus);
      gtk_pointer_focus_unref (focus);
    }
}

void
gtk_root_set_pointer_focus_grab (GtkRoot          *root,
                                 GdkDevice        *device,
                                 GdkEventSequence *sequence,
                                 GtkWidget        *grab_widget)
{
  GtkPointerFocus *focus;
  GtkWidget *current;

  focus = gtk_root_lookup_pointer_focus (root, device, sequence);
  if (!focus && !grab_widget)
    return;

  current = gtk_pointer_focus_get_implicit_grab (focus);

  if (current == grab_widget)
    return;

  if (current)
    set_widget_active_state (current, NULL, FALSE);

  gtk_pointer_focus_set_implicit_grab (focus, grab_widget);

  if (grab_widget)
    set_widget_active_state (grab_widget, NULL, TRUE);
}

void
gtk_root_update_pointer_focus_state_change (GtkRoot   *root,
                                            GtkWidget *widget)
{
  Foci *f = gtk_root_get_foci (root);
  GList *l;

  l = f->pointer_foci;
  while (l)
    {
      GList *next;

      GtkPointerFocus *focus = l->data;

      next = l->next;

      gtk_pointer_focus_ref (focus);

      if (focus->grab_widget &&
          (focus->grab_widget == widget ||
           gtk_widget_is_ancestor (focus->grab_widget, widget)))
        {
          set_widget_active_state (focus->grab_widget, widget, FALSE);
          gtk_pointer_focus_set_implicit_grab (focus, gtk_widget_get_parent (widget));
        }

      if (GTK_WIDGET (focus->root) == widget)
        {
          /* Unmapping the toplevel, remove pointer focus */
          f->pointer_foci = g_list_remove_link (f->pointer_foci, l);
          gtk_pointer_focus_unref (focus);
          g_list_free (l);
        }
      else if (focus->target == widget ||
               gtk_widget_is_ancestor (focus->target, widget))
        {
          GtkWidget *old_target;

          old_target = g_object_ref (focus->target);
          gtk_pointer_focus_repick_target (focus);
          if (gtk_widget_get_native (focus->target) == gtk_widget_get_native (old_target))
            {
              gtk_synthesize_crossing_events (root,
                                              GTK_CROSSING_POINTER,
                                              old_target, focus->target,
                                              focus->x, focus->y,
                                              GDK_CROSSING_NORMAL,
                                              NULL);
            }
          g_object_unref (old_target);
        }

      gtk_pointer_focus_unref (focus);

      l = next;
    }
}

void
gtk_root_maybe_revoke_implicit_grab (GtkRoot   *root,
                                     GdkDevice *device,
                                     GtkWidget *grab_widget)
{
  Foci *f = gtk_root_get_foci (root);

  for (GList *l = f->pointer_foci; l; l = l->next)
    {
      GtkPointerFocus *focus = l->data;

      if ((!device || focus->device == device) &&
          focus->target != grab_widget &&
          !gtk_widget_is_ancestor (focus->target, grab_widget))
        gtk_root_set_pointer_focus_grab (root, focus->device, focus->sequence, NULL);
    }
}

static void
gtk_root_update_cursor (GtkRoot   *root,
                        GdkDevice *device,
                        GtkWidget *grab_widget,
                        GtkWidget *target)
{
  g_return_if_fail (GTK_IS_WINDOW (root));

  gtk_window_update_cursor (GTK_WINDOW (root), device, grab_widget, target);
}

void
gtk_root_maybe_update_cursor (GtkRoot   *root,
                              GtkWidget *widget,
                              GdkDevice *device)
{
  Foci *f = gtk_root_get_foci (root);

  for (GList *l = f->pointer_foci; l; l = l->next)
    {
      GtkPointerFocus *focus = l->data;
      GtkWidget *grab_widget = NULL;
      GtkWidget *target;

      if (focus->sequence)
        continue;
      if (device && device != focus->device)
        continue;

      if (GTK_IS_WINDOW (root))
        {
          GtkWindowGroup *group = gtk_window_get_group (GTK_WINDOW (root));
          grab_widget = gtk_window_group_get_current_grab (group);
        }

      if (!grab_widget)
        grab_widget = gtk_pointer_focus_get_implicit_grab (focus);

      target = gtk_pointer_focus_get_target (focus);

      if (widget)
        {
          /* Check whether the changed widget affects current cursor lookups. */
          if (grab_widget && grab_widget != widget &&
              !gtk_widget_is_ancestor (widget, grab_widget))
            continue;
          if (target != widget &&
              !gtk_widget_is_ancestor (target, widget))
            continue;
        }

      gtk_root_update_cursor (focus->root, focus->device, grab_widget, target);

      if (device)
        break;
    }
}

void
gtk_root_device_removed (GtkRoot   *root,
                         GdkDevice *device)
{
  Foci *f = gtk_root_get_foci (root);
  GList *l;

  l = f->pointer_foci;
  while (l)
    {
      GList *next;
      GtkPointerFocus *focus = l->data;

      next = l->next;

      if (focus->device == device)
        {
          f->pointer_foci = g_list_delete_link (f->pointer_foci, l);
          gtk_pointer_focus_unref (focus);
        }
      l = next;
    }
}

GdkDevice **
gtk_root_get_foci_on_widget (GtkRoot      *root,
                             GtkWidget    *widget,
                             unsigned int *n_devices)
{
  Foci *f = gtk_root_get_foci (root);
  GPtrArray *array = g_ptr_array_new ();

  for (GList *l = f->pointer_foci; l; l = l->next)
    {
      GtkPointerFocus *focus = l->data;
      GtkWidget *target;

      target = gtk_pointer_focus_get_effective_target (focus);

      if (target == widget || gtk_widget_is_ancestor (target, widget))
        g_ptr_array_add (array, focus->device);
    }

  if (n_devices)
    *n_devices = array->len;

  return (GdkDevice **) g_ptr_array_free (array, FALSE);
}

static void
gtk_synthesize_grab_crossing (GtkWidget *child,
                              GdkDevice *device,
                              GtkWidget *new_grab,
                              GtkWidget *old_grab,
                              gboolean   from_grab,
                              gboolean   was_shadowed,
                              gboolean   is_shadowed)
{
  g_object_ref (child);

  if (is_shadowed)
    {
      if (!was_shadowed && gtk_widget_is_sensitive (child))
        _gtk_widget_synthesize_crossing (child,
                                         new_grab,
                                         device,
                                         GDK_CROSSING_GTK_GRAB);
    }
  else
    {
      if (was_shadowed && gtk_widget_is_sensitive (child))
        _gtk_widget_synthesize_crossing (old_grab, child,
                                         device,
                                         from_grab ? GDK_CROSSING_GTK_GRAB :
                                         GDK_CROSSING_GTK_UNGRAB);
    }

  g_object_unref (child);
}

static void
gtk_root_propagate_grab_notify (GtkRoot   *root,
                                GtkWidget *target,
                                GdkDevice *device,
                                GtkWidget *old_grab,
                                GtkWidget *new_grab,
                                gboolean   from_grab)
{
  GList *l, *widgets = NULL;
  gboolean was_grabbed = FALSE, is_grabbed = FALSE;

  while (target)
    {
      if (target == old_grab)
        was_grabbed = TRUE;
      if (target == new_grab)
        is_grabbed = TRUE;
      widgets = g_list_prepend (widgets, g_object_ref (target));
      target = gtk_widget_get_parent (target);
    }

  widgets = g_list_reverse (widgets);

  for (l = widgets; l; l = l->next)
    {
      gboolean was_shadowed, is_shadowed;

      was_shadowed = old_grab && !was_grabbed;
      is_shadowed = new_grab && !is_grabbed;

      if (l->data == old_grab)
        was_grabbed = FALSE;
      if (l->data == new_grab)
        is_grabbed = FALSE;

      if (was_shadowed == is_shadowed)
        break;

      gtk_synthesize_grab_crossing (l->data, device, old_grab, new_grab, from_grab, was_shadowed, is_shadowed);

      gtk_widget_reset_controllers (l->data);
    }

  g_list_free_full (widgets, g_object_unref);
}

void
gtk_root_grab_notify (GtkRoot   *root,
                      GtkWidget *old_grab,
                      GtkWidget *new_grab,
                      gboolean   from_grab)
{
  Foci *f = gtk_root_get_foci (root);

  for (GList *l = f->pointer_foci; l; l = l->next)
    {
      GtkPointerFocus *focus = l->data;

      gtk_root_propagate_grab_notify (root,
                                      gtk_pointer_focus_get_effective_target (focus),
                                      focus->device,
                                      old_grab,
                                      new_grab,
                                      from_grab);
    }
}
