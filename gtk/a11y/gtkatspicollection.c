/* gtkatspicollection.c: Implement ATSPI's Collection interface
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2025 <two@envs.net>
 */
#include "a11y/gtkatspicontextprivate.h"
#include "a11y/gtkatspiprivate.h"
#include "a11y/gtkatspiutilsprivate.h"
#include "gio/gio.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtkaccessibleprivate.h"
#include "gtkatcontextprivate.h"
#include "gtkatspicollectionprivate.h"
#include <stdio.h>

struct Rule {
  uint64_t states;
  AtspiCollectionMatchType match_states;
  GVariant *attrs;
  AtspiCollectionMatchType match_attrs;
  uint32_t roles[(ATSPI_ROLE_LAST_DEFINED + 31) / 32];
  AtspiCollectionMatchType match_roles;
  GVariant *ifaces;
  AtspiCollectionMatchType match_ifaces;
  gboolean invert;
};

static void
rule_finalize (struct Rule *rule)
{
  g_clear_pointer(&rule->ifaces, g_variant_unref);
  g_clear_pointer(&rule->attrs, g_variant_unref);
}

static bool
parse_rule (GVariant *v,
            GDBusMethodInvocation *invocation,
            struct Rule *ret)
{
  *ret = (struct Rule){ 0, };

  g_autoptr (GVariantIter) states_v = NULL, roles_v = NULL;
  g_variant_get (v, "(aii@a{ss}iaii@asib)",
                 &states_v, &ret->match_states,
                 &ret->attrs, &ret->match_attrs,
                 &roles_v, &ret->match_roles,
                 &ret->ifaces, &ret->match_ifaces,
                 &ret->invert);

  // check if MatchType enum values are valid
  AtspiCollectionMatchType match_modes[] = { ret->match_ifaces, ret->match_attrs, ret->match_ifaces, ret->match_roles };
  for (int i = 0; i < G_N_ELEMENTS(match_modes); i++)
    {
      AtspiCollectionMatchType m = match_modes[i];
      if ((m < ATSPI_COLLECTION_MATCH_ALL) || (m > ATSPI_COLLECTION_MATCH_EMPTY))
        {
          g_dbus_method_invocation_return_error_literal (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "invalid value for AtspiCollectionMatchType");
          return false;
        }
    }

  // AT-SPI represents bit-masks for states and roles as arrays of i32

  // states can be packed into a u64
  uint32_t tmp; // we read the i32 into a u32 because bitmath on signed values is scary
  if (!g_variant_iter_next (states_v, "i", &tmp))
    {
      ret->states |= tmp;
      if (!g_variant_iter_next (states_v, "i", &tmp))
        {
          ret->states |= ((uint64_t) tmp) << 32;
          if (ret->match_states == ATSPI_COLLECTION_MATCH_ALL &&
              g_variant_iter_next(states_v, "i", &tmp) &&
              tmp != 0) {
            // client requested roles beyond what gtk knows about ...
            // TODO: reject this
          }
        }
    }
  for (int i = 0; i < G_N_ELEMENTS (ret->roles); i++)
    {
      if (!g_variant_iter_next (roles_v, "i", &ret->roles[i]))
        break;
      // TODO: reject if too many
    }
  return true;
}

static bool
match_ifaces (GVariant *criteria, AtspiCollectionMatchType how, GVariant *what)
{
  switch (how)
    {
    case ATSPI_COLLECTION_MATCH_ALL:
      {
        GVariantIter cr_iter;
        g_variant_iter_init (&cr_iter, criteria);
        const char *criterion;
        while (g_variant_iter_next (&cr_iter, "s", &criterion))
          {
            GVariantIter iface_iter;
            g_variant_iter_init (&iface_iter, what);
            const char *one;
            bool matched = false;
            while (g_variant_iter_next (&iface_iter, "s", &one))
              {
                if (g_strcmp0 (criterion, one) == 0)
                  matched = true;
              }
            if (!matched)
              return false;
          }
        return true;
      }
    case ATSPI_COLLECTION_MATCH_EMPTY: // XXX: check if empty
    case ATSPI_COLLECTION_MATCH_ANY:
      {
        GVariantIter iface_iter;
        g_variant_iter_init (&iface_iter, what);
        const char *one;
        while (g_variant_iter_next (&iface_iter, "s", &one))
          {
            GVariantIter cr_iter;
            g_variant_iter_init (&cr_iter, criteria);
            const char *criterion;
            while (g_variant_iter_next (&cr_iter, "s", &criterion))
              {
                if (g_strcmp0 (criterion, one) == 0)
                  return true;
              }
          }
        return false;
      }
    case ATSPI_COLLECTION_MATCH_NONE:
      return !match_ifaces(criteria, ATSPI_COLLECTION_MATCH_ALL, what);
    default:
      g_assert_not_reached();
    }
}

static bool
rule_match (const struct Rule *r, GtkATContext *elem)
{
  bool match = true;
  {
    // XXX: if not NONE, always behaves as ANY
    // well its not like other modes make sense for roles, of which there can only be one ...
    // (right ?)
    AtspiRole elem_role = gtk_atspi_role_for_context(elem);
    bool role_p = false;
    for (AtspiRole role = 0; role < ATSPI_ROLE_LAST_DEFINED; role++)
      {
        bool is_in_rule = r->roles[role / 32] >> (role % 32) & 1;
        if (is_in_rule && elem_role == role)
          role_p |= true;
      }
    role_p ^= r->match_roles == ATSPI_COLLECTION_MATCH_NONE;
    match &= role_p;
  }
  {
    uint64_t elem_states = gtk_at_spi_context_get_states_as_u64 (GTK_AT_SPI_CONTEXT (elem));
    uint64_t matched_states = elem_states & r->states;
    switch (r->match_states) {
      case ATSPI_COLLECTION_MATCH_ALL:
        match &= matched_states == r->states;
        break;
      case ATSPI_COLLECTION_MATCH_ANY:
        match &= !!matched_states;
        break;
      case ATSPI_COLLECTION_MATCH_NONE:
        match &= matched_states == 0;
        break;
      case ATSPI_COLLECTION_MATCH_EMPTY: // why does this even exist ??
        match &= r->match_states && !!matched_states;
        break;
      default:
        break;
      }
  }

  GVariant *interfaces /*unowned*/ = gtk_at_spi_context_get_interfaces (GTK_AT_SPI_CONTEXT (elem));
  match &= match_ifaces(r->ifaces, r->match_ifaces, interfaces);

  // TODO: match attrs
  return match ^ r->invert;
}

static void
collect_matches (GtkAccessible *parent,
                 const struct Rule *rule,
                 AtspiCollectionSortOrder sortby,
                 int limit,
                 GVariantBuilder *ret)
{
  for (GtkAccessible *child = gtk_accessible_get_first_accessible_child (parent);
       child != NULL;
       child = gtk_accessible_get_next_accessible_sibling (child))
    {
      if (limit < 1)
        break;

      if (!gtk_accessible_should_present (child))
        {
          g_object_unref (child);
          continue;
        }

      GtkATContext *context = gtk_accessible_get_at_context (child);
      if (rule_match (rule, context))
        {
          /* as in ./gtkatspicontext.c */
          /* Realize the child ATContext in order to get its ref */
          gtk_at_context_realize (context);
          g_variant_builder_add_value (ret, gtk_at_spi_context_to_ref (GTK_AT_SPI_CONTEXT (context)));
          limit--;
        }

      g_object_unref (child);
    }

  // TODO: also sort it correctly by the passed order
}

static void
collection_handle_method (GDBusConnection       *connection,
                         const gchar           *sender,
                         const gchar           *object_path,
                         const gchar           *interface_name,
                         const gchar           *method_name,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation,
                         gpointer               user_data)
{
  GtkAtSpiContext *self = user_data;
  GtkAccessible *accessible = gtk_at_context_get_accessible ((GtkATContext*)self);

  if (g_strcmp0 (method_name, "GetMatches") == 0)
    {
      GVariantBuilder ret;
      g_variant_builder_init (&ret, G_VARIANT_TYPE ("a(so)"));

      AtspiCollectionSortOrder sortby;
      GVariant *rule_v;
      int limit;
      gboolean traverse;
      g_variant_get (parameters, "(ruib)", &rule_v, &sortby, &limit, &traverse);

      // from orca code
      // > The final argument, traverse, is not supported but is expected
      (void)traverse;
      struct Rule rule;
      if (parse_rule (rule_v, invocation, &rule))
        {
          collect_matches (accessible, &rule, sortby, limit, &ret);

          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(@a(so))", g_variant_builder_end (&ret)));
        }
      rule_finalize(&rule);
    }
  else if (g_strcmp0 (method_name, "GetMatchesTo") == 0 ||
           g_strcmp0 (method_name, "GetMatchesFrom") == 0)
    {
      // TODO: find some client that actually uses those (orca doesnt), implement and test with that client
      g_dbus_method_invocation_return_error_literal (invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
      // empty error, same as ./gtkatspicomponent.c does
    }
  else if (g_strcmp0 (method_name, "GetActiveDescendant") == 0)
    {
      // XXX: ???
      // in at-spi2-core docs :
      // >Returns the active descendant of the given object. Not currently implemented in libatspi.
      g_dbus_method_invocation_return_error_literal (invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}
static const GDBusInterfaceVTable collection_vtable = {
  collection_handle_method,
};

const GDBusInterfaceVTable *
gtk_atspi_get_collection_vtable (GtkAccessible *accessible)
{
  return &collection_vtable;
}
