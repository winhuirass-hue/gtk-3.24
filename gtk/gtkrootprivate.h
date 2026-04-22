#pragma once

#include "gtkroot.h"

#include "gtkconstraintsolverprivate.h"

G_BEGIN_DECLS

/**
 * GtkRootIface:
 *
 * The list of functions that must be implemented for the `GtkRoot` interface.
 */
struct _GtkRootInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  GdkDisplay * (* get_display)  (GtkRoot *self);

  GtkConstraintSolver * (* get_constraint_solver) (GtkRoot *self);

  GtkWidget *  (* get_focus)    (GtkRoot   *self);
  void         (* set_focus)    (GtkRoot   *self,
                                 GtkWidget *focus);

};

GtkConstraintSolver *   gtk_root_get_constraint_solver  (GtkRoot *self);

void         gtk_root_start_layout  (GtkRoot *self);
void         gtk_root_stop_layout   (GtkRoot *self);
void         gtk_root_queue_restyle (GtkRoot *self);


GtkWidget *  gtk_root_lookup_pointer_focus               (GtkRoot          *root,
                                                          GdkDevice        *device,
                                                          GdkEventSequence *sequence);
GtkWidget *  gtk_root_lookup_effective_pointer_focus     (GtkRoot          *root,
                                                          GdkDevice        *device,
                                                          GdkEventSequence *sequence);
GtkWidget *  gtk_root_lookup_pointer_focus_implicit_grab (GtkRoot          *rootm,
                                                          GdkDevice        *device,
                                                          GdkEventSequence *sequence);

void         gtk_root_update_pointer_focus               (GtkRoot          *root,
                                                          GdkDevice        *device,
                                                          GdkEventSequence *sequence,
                                                          GtkWidget        *target,
                                                          double            x,
                                                          double            y);
void         gtk_root_set_pointer_focus_grab             (GtkRoot          *root,
                                                          GdkDevice        *device,
                                                          GdkEventSequence *sequence,
                                                          GtkWidget        *grab_widget);

void         gtk_root_update_pointer_focus_state_change  (GtkRoot          *root,
                                                          GtkWidget        *widget);

void         gtk_root_maybe_revoke_implicit_grab         (GtkRoot          *root,
                                                          GdkDevice        *device,
                                                          GtkWidget        *grab_widget);

void         gtk_root_maybe_update_cursor                (GtkRoot          *root,
                                                          GtkWidget        *widget,
                                                          GdkDevice        *device);

G_END_DECLS

