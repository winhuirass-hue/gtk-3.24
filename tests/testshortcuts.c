#include <gtk/gtk.h>
#include <stdint.h>
#include <stdio.h>

static void
quit_cb (GtkWidget *widget,
         gpointer data)
{
  gboolean *done = data;
  *done = TRUE;
  g_main_context_wakeup (NULL);
}

static int64_t mask = 0;

static gboolean
reset_mask (GtkWidget *widget,
            GVariant *args,
            gpointer data)
{
  mask = 0;
  return true;
}

struct ShortcutConfig
{
  const char *trigger_string;
  guint trigger_key;
  GdkModifierType trigger_modifiers;
  const char *message;
  GtkShortcutTrigger *trigger;
};

struct ShortcutConfig shortcuts[] = {
  { "z", 0, 0, "z" },
  { "<Control>z", 0, 0, "<Control>z" },
  { "<Shift>z", 0, 0, "<Shift>z" },
  { "<Control><Shift>Z", 0, 0, "<Control><Shift>z" },
  { "<Alt>z", 0, 0, "<Alt>z" },
  { "<Control><Alt>z", 0, 0, "<Control><Alt>z" },
  { "<Shift><Alt>z", 0, 0, "<Shift><Alt>z" },
  { "<Control><Shift><Alt>z", 0, 0, "<Control><Shift><Alt>z" },

  { "<Super>z", 0, 0, "<Super>z" },
  { "<Super><Control>z", 0, 0, "<Super><Control>z" },
  { "<Super><Shift>z", 0, 0, "<Super><Shift>z" },
  { "<Super><Control><Shift>z", 0, 0, "<Super><Control><Shift>z" },
  { "<Super><Alt>z", 0, 0, "<Super><Alt>z" },
  { "<Super><Control><Alt>z", 0, 0, "<Super><Control><Alt>z" },
  { "<Super><Shift><Alt>z", 0, 0, "<Super><Shift><Alt>z" },
  { "<Super><Control><Shift><Alt>z", 0, 0, "<Super><Control><Shift><Alt>z" },

  { "<Meta>z", 0, 0, "<Meta>z" },
  { "<Meta><Control>z", 0, 0, "<Meta><Control>z" },
  { "<Meta><Shift>z", 0, 0, "<Meta><Shift>z" },
  { "<Meta><Control><Shift>z", 0, 0, "<Meta><Control><Shift>z" },
  { "<Meta><Alt>z", 0, 0, "<Meta><Alt>z" },
  { "<Meta><Control><Alt>z", 0, 0, "<Meta><Control><Alt>z" },
  { "<Meta><Shift><Alt>z", 0, 0, "<Meta><Shift><Alt>z" },
  { "<Meta><Control><Shift><Alt>z", 0, 0, "<Meta><Control><Shift><Alt>z" },

  { NULL, GDK_KEY_equal, GDK_NO_MODIFIER_MASK, "=" },
  { NULL, GDK_KEY_plus, GDK_NO_MODIFIER_MASK, "+" },
  { NULL, GDK_KEY_equal, GDK_SHIFT_MASK, "<Shift>=" },
  { NULL, GDK_KEY_plus, GDK_SHIFT_MASK, "<Shift>+" },

  { NULL, GDK_KEY_equal, GDK_CONTROL_MASK, "<Control>=" },
  { NULL, GDK_KEY_plus, GDK_CONTROL_MASK, "<Control>+" },
  { NULL, GDK_KEY_equal, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "<Control><Shift>=" },
  { NULL, GDK_KEY_plus, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "<Control><Shift>+" },

  { NULL, GDK_KEY_equal, GDK_SUPER_MASK, "<Super>=" },
  { NULL, GDK_KEY_plus, GDK_SUPER_MASK, "<Super>+" },
  { NULL, GDK_KEY_equal, GDK_SUPER_MASK | GDK_SHIFT_MASK, "<Super><Shift>=" },
  { NULL, GDK_KEY_plus, GDK_SUPER_MASK | GDK_SHIFT_MASK, "<Super><Shift>+" },

  { NULL, GDK_KEY_equal, GDK_META_MASK, "<Meta>=" },
  { NULL, GDK_KEY_plus, GDK_META_MASK, "<Meta>+" },
  { NULL, GDK_KEY_equal, GDK_META_MASK | GDK_SHIFT_MASK, "<Meta><Shift>=" },
  { NULL, GDK_KEY_plus, GDK_META_MASK | GDK_SHIFT_MASK, "<Meta><Shift>+" },

  { NULL, GDK_KEY_at, GDK_NO_MODIFIER_MASK, "@" },
};

static gboolean
print_cb (GtkWidget *widget,
          GVariant *args,
          gpointer data)
{
  struct ShortcutConfig *shortcut = data;
  int index = shortcut - shortcuts;
  mask |= (1ULL << index);
  fprintf (stderr, "%s index:%d m:%" PRIX64 "\n", shortcut->message, index, mask);
  return true;
}

static gboolean
key_pressed_cb (GtkEventControllerKey *controller,
                guint keyval,
                guint keycode,
                GdkModifierType modifiers,
                gpointer user_data)
{
  int i;
  int total_match = 0;
  for (i = 0; i < G_N_ELEMENTS (shortcuts); i++)
    {
      GdkKeyMatch match = gtk_shortcut_trigger_trigger (shortcuts[i].trigger,
                                                        gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller)), false);
      if (match)
        {
          GString *str = g_string_new ("");
          gtk_shortcut_trigger_print (shortcuts[i].trigger, str);
          fprintf (stderr, "match: %s -> %s %s\n",
                   (match == GDK_KEY_MATCH_PARTIAL) ? "partial" : "exact",
                   str->str, shortcuts[i].message);
          total_match += 1;
        }
    }
  if (total_match > 1)
    {
      fprintf (stderr, "Multiple match:%d\n", total_match);
    }
  return FALSE;
}

int
main (int argc, char **argv)
{
  gboolean done = false;
  GtkWidget *window;

  gtk_init ();

  window = gtk_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  gtk_window_present (GTK_WINDOW (window));

  GtkEventControllerKey *c0 = GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  g_signal_connect (c0, "key-pressed", G_CALLBACK (key_pressed_cb), NULL);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (c0), GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (window, GTK_EVENT_CONTROLLER (c0));

  GtkShortcutController *controller = GTK_SHORTCUT_CONTROLLER (gtk_shortcut_controller_new ());
  gtk_widget_add_controller (window, GTK_EVENT_CONTROLLER (controller));

  int i;
  for (i = 0; i < G_N_ELEMENTS (shortcuts); i++)
    {
      GtkShortcutTrigger *trigger = NULL;
      if (shortcuts[i].trigger_string)
        {
          trigger = gtk_shortcut_trigger_parse_string (shortcuts[i].trigger_string);
        }
      else
        {
          trigger = gtk_keyval_trigger_new (shortcuts[i].trigger_key, shortcuts[i].trigger_modifiers);
        }
      shortcuts[i].trigger = trigger;
      GtkShortcut *shortcut = gtk_shortcut_new (
          trigger,
          gtk_callback_action_new (print_cb, &shortcuts[i], NULL));
      gtk_shortcut_controller_add_shortcut (controller, shortcut);
    }
  {
    GtkShortcut *shortcut = gtk_shortcut_new (
        gtk_shortcut_trigger_parse_string ("0"),
        gtk_callback_action_new (reset_mask, NULL, NULL));
    gtk_shortcut_controller_add_shortcut (controller, shortcut);
  }

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
