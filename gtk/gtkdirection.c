/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2026 Hari Rana <theevilskeleton@riseup.net>
 */

#include "config.h"

#include "gtkdirection.h"


/**
 * gtk_keyval_to_physical_direction:
 * @keyval: the GDK key symbol
 *
 * Returns the corresponding physical direction provided by @keyval.
 *
 * If @keyval does not correspond to a physical direction,
 * then `-1` is returned.
 *
 * ::: seealso
 *
 *     [Keyboard Codes, Groups, And Modifiers](https://docs.gtk.org/gdk4/keys.html)
 *
 * Returns: the corresponding direction, or `-1`
 *
 * Since: 4.24.0
 */
GtkDirectionType
gtk_keyval_to_physical_direction (guint keyval)
{
  switch (keyval)
    {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      return GTK_DIR_UP;

    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      return GTK_DIR_RIGHT;

    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      return GTK_DIR_DOWN;

    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      return GTK_DIR_LEFT;

    default:
      return -1;
    }
}

