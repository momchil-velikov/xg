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
#include <ulib/bitset.h>

/* LR(0) set cache.  */
static ulib_cache *lr0set_cache;

/* LR(0) set constructor.  */
static int
lr0set_ctor (xg_lr0set *set, unsigned int size __attribute__ ((unused)))
{
  if (ulib_vector_init (&set->items, ULIB_ELT_SIZE, sizeof (xg_lr0item), 0)
      < 0)
    return -1;
  else
    return 0;
}

/* LR(0) set clear.  */
static void
lr0set_clear (xg_lr0set *set, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_set_size (&set->items, 0);
}

/* LR(0) set destructor.  */
static void
lr0set_dtor (xg_lr0set *set, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_destroy (&set->items);
}


/* Create an LR(0) set.  */
xg_lr0set *
xg_lr0set_new ()
{
  xg_lr0set *set;

  if ((set = ulib_cache_alloc (lr0set_cache)) != 0)
    return set;

  ulib_log_printf (xg_log, "ERROR: Unable to allocate an LR(0) set");
  return 0;
}

/* Add the item <PROD,DOT> to the set, if not already present.  Return
   negative on error, positive if the set changed (item not present)
   or zero otherwise.  */
int
xg_lr0set_add_item (xg_lr0set *set, unsigned int prod, unsigned int dot)
{
  unsigned int i, n;
  xg_lr0item *it;

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
xg_lr0set_count (const xg_lr0set *set)
{
  return ulib_vector_length (&set->items);
}

/* Return the Nth item in the set.  */
const xg_lr0item *
xg_lr0set_get_item (const xg_lr0set *set, unsigned int n)
{
  return ulib_vector_elt (&set->items, n);
}

/* Compute the closure of an LR(0) set.  */
static int
lr0set_closure (const xg_grammar *g, xg_lr0set *set, ulib_bitset *done)
{
  unsigned int i, j, n;
  xg_sym sym;
  xg_symdef *def;
  const xg_lr0item *it;
  const xg_prod *p;
 
  /* Get the next LR(0) item.  If the dot is in front of a terminal,
     skip the item, otherwise add to the set an item with the dot at
     the front for each production having the symbol as its left hand
     side.  Do not add duplicate items.  */
  for (i = 0; i < xg_lr0set_count (set); ++i)
    {
      /* Get next item, the production and the symbol following the
         dot.  */
      it = xg_lr0set_get_item (set, i);
      p = xg_grammar_get_prod (g, it->prod);

      /* Do nothing if the dot is at the end of the production, in
         front of a terninal symbol or in from the an already expanded
         non-terminal.  */
      if (it->dot >= xg_prod_length (p))
        continue;
      sym = xg_prod_get_symbol (p, it->dot);
      if (xg_grammar_is_terminal_sym (g, sym) || ulib_bitset_is_set (done, sym))
        continue;

      /* Add the non-terminal to the done set.  */
      if (ulib_bitset_set (done, sym) < 0)
        {
          ulib_log_printf (xg_log,
                           "ERROR: Unable to add to the done set while"
                           " computing LR(0) closure");
          return -1;
        }

      /* Append items LR(0) items.  */
      def = xg_grammar_get_symbol (g, sym);
      n = xg_symdef_prod_count (def);
      for (j = 0; j < n; ++j)
        if (xg_lr0set_add_item (set, xg_symdef_get_prod (def, j), 0) < 0)
          return -1;
    }

  return 0;
}

/* Compute the closure of an LR(0) set.  */
int
xg_lr0set_closure (const xg_grammar *g, xg_lr0set *set)
{
  int sts;
  ulib_bitset done;

  /* Create a set record already expanded non-terminal symbols.  */
  if (ulib_bitset_init (&done) < 0)
    {
      ulib_log_printf (xg_log,
                       "ERROR: Unable to create expansion track bitset.");
      return -1;
    }

  sts = lr0set_closure(g, set, &done);
  
  ulib_bitset_destroy (&done);
  
  return sts;
}

/* Display a debugging dump of an LR(0) set.  */
void
xg_lr0set_debug (FILE *out, const struct xg_grammar *g,
                  const xg_lr0set *set)
{
  unsigned int i, j, n, m;
  xg_prod *p;
  const xg_lr0item *it;

  n = xg_lr0set_count (set);
  for (i = 0; i < n; ++i)
    {
      it = xg_lr0set_get_item (set, i);
      p = xg_grammar_get_prod (g, it->prod);

      xg_symbol_name_debug (out, g, p->lhs);
      fputs (" ->", out);

      for (j = 0; j < it->dot; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_prod_get_symbol (p, j));
        }

      fputs (" .", out);

      m = xg_prod_length (p);
      for (;j < m; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_prod_get_symbol (p, j));
        }

      fputc ('\n', out);
    }
}


/* Initialize LR(0) sets memory management.  */
int
xg__init_lr0sets ()
{
  lr0set_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_lr0set),
                                    ULIB_CACHE_ALIGN, sizeof (void *),
                                    ULIB_CACHE_CTOR, lr0set_ctor,
                                    ULIB_CACHE_CLEAR, lr0set_clear,
                                    ULIB_CACHE_DTOR, lr0set_dtor,
                                    ULIB_CACHE_GC,
                                    0);
  if (lr0set_cache)
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
