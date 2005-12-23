/* lr0.c - LR(0) set operation
 *
 * Copyright (C) 2005 Momchil Velikov
 *
 * This file is part of XG.
 *
 * XG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XG; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lr0.h"
#include "grammar.h"
#include "xg.h"
#include <ulib/cache.h>
#include <ulib/log.h>

/* LR(0) set cache.  */
static ulib_cache *lr0_set_cache;

/* LR(0) set constructor.  */
static int
lr0_set_ctor (xg_lr0_set *set, unsigned int size __attribute__ ((unused)))
{
  if (ulib_vector_init (&set->items, ULIB_ELT_SIZE, sizeof (xg_lr0_item), 0)
      < 0)
    return -1;
  else
    return 0;
}

/* LR(0) set clear.  */
static void
lr0_set_clear (xg_lr0_set *set, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_set_size (&set->items, 0);
}

/* LR(0) set destructor.  */
static void
lr0_set_dtor (xg_lr0_set *set, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_destroy (&set->items);
}


/* Create an LR(0) set.  */
xg_lr0_set *
xg_lr0_set_new ()
{
  xg_lr0_set *set;

  if ((set = ulib_cache_alloc (lr0_set_cache)) != 0)
    return set;

  ulib_log_printf (xg_log, "ERROR: Unable to allocate an LR(0) set");
  return 0;
}

/* Add the item <PROD,DOT> to the set, if not already present.  Return
   negative on error, positive if the set changed (item not present)
   or zero otherwise.  */
int
xg_lr0_set_add (xg_lr0_set *set, unsigned int prod, unsigned int dot)
{
  unsigned int i, n;
  xg_lr0_item *it;

  /* Check for duplicates. */
  n = ulib_vector_length (&set->items);
  it = ulib_vector_front (&set->items);
  for (i = 0; i < n; ++i, ++it)
    if (it->prod == prod && it->dot == dot)
      return 0;

  /* Append the new item.  */
  if (ulib_vector_resize (&set->items, 1) == 0)
    {
      it = ulib_vector_back (&set->items);

      it [-1].prod = prod;
      it [-1].dot = dot;

      return 1;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to append an LR(0) item");
  return -1;
}

/* Return the number of items in the set.  */
unsigned int
xg_lr0_set_count (const xg_lr0_set *set)
{
  return ulib_vector_length (&set->items);
}

/* Return the Nth item in the set.  */
const xg_lr0_item *
xg_lr0_set_get (const xg_lr0_set *set, unsigned int n)
{
  return ulib_vector_elt (&set->items, n);
}

/* Compute the closure of an LR(0) set.  */
int
xg_lr0_set_closure (const xg_grammar *g, xg_lr0_set *set)
{
  unsigned int i, j, n;
  xg_symbol sym;
  xg_symbol_def *def;
  const xg_lr0_item *it;
  const xg_production *p;

  /* Get the next LR(0) item.  If the dot is in front of a terminal,
     skip the item, otherwise add to the set an item with the dot at
     the front for each production having the symbol as its left hand
     side.  Do not add duplicate items.  */
  i = 0;
  while (i < xg_lr0_set_count (set))
    {
      /* Get next item, the production and the symbol following the
         dot.  */
      it = xg_lr0_set_get (set, i);
      p = xg_grammar_get_production (g, it->prod);
      if (it->dot < xg_production_length (p))
        {
          sym = xg_production_get_symbol (p, it->dot);
          if (!xg_grammar_is_terminal_sym (g, sym))
            {
              def = xg_grammar_get_symbol (g, sym);

              /* Append items.  */
              n = xg_symbol_def_production_count (def);
              for (j = 0; j < n; ++j)
                if (xg_lr0_set_add (set, xg_symbol_def_get_production (def, j), 0) < 0)
                  return -1;
            }
        }

      /* Advance to the next unprocessed item.  */
      ++i;
    }

  return 0;
}

/* Display a debugging dump of an LR(0) set.  */
void
xg_lr0_set_debug (FILE *out, const struct xg_grammar *g,
                  const xg_lr0_set *set)
{
  unsigned int i, j, n, m;
  xg_production *p;
  const xg_lr0_item *it;

  n = xg_lr0_set_count (set);
  for (i = 0; i < n; ++i)
    {
      it = xg_lr0_set_get (set, i);
      p = xg_grammar_get_production (g, it->prod);

      xg_symbol_name_debug (out, g, p->lhs);
      fputs (" ->", out);

      for (j = 0; j < it->dot; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_production_get_symbol (p, j));
        }

      fputs (" .", out);

      m = xg_production_length (p);
      for (;j < m; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_production_get_symbol (p, j));
        }

      fputc ('\n', out);
    }
}


/* Initialize LR(0) sets memory management.  */
int
xg__init_lr0_sets ()
{
  lr0_set_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_lr0_set),
                                     ULIB_CACHE_ALIGN, sizeof (void *),
                                     ULIB_CACHE_CTOR, lr0_set_ctor,
                                     ULIB_CACHE_CLEAR, lr0_set_clear,
                                     ULIB_CACHE_DTOR, lr0_set_dtor,
                                     ULIB_CACHE_GC,
                                     0);
  if (lr0_set_cache)
    return 0;
  else
    {
      ulib_log_printf (xg_log, "ERROR: Unable to create the LR(0) sets cache");
      return -1;
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 11160ca5-723b-49b2-a94c-8d4b3919b35b
 * End:
 */
