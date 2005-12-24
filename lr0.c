/* lr0.c - LR(0) DFA operation
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

/* LR(0) states cache.  */
static ulib_cache *lr0state_cache;

/* LR(0) state constructor.  */
static int
lr0state_ctor (xg_lr0state *state, unsigned int size __attribute__ ((unused)))
{
  if (ulib_vector_init (&state->items, ULIB_ELT_SIZE, sizeof (xg_lr0item), 0)
      < 0)
    return -1;
  else
    return 0;
}

/* LR(0) state clear.  */
static void
lr0state_clear (xg_lr0state *state, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_set_size (&state->items, 0);
}

/* LR(0) state destructor.  */
static void
lr0state_dtor (xg_lr0state *state, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_destroy (&state->items);
}


/* Create an LR(0) state.  */
xg_lr0state *
xg_lr0state_new ()
{
  xg_lr0state *state;

  if ((state = ulib_cache_alloc (lr0state_cache)) != 0)
    return state;

  ulib_log_printf (xg_log, "ERROR: Unable to allocate an LR(0) state");
  return 0;
}

/* Add the item <PROD,DOT> to the state, if not already present.
   Return negative on error, positive if the state changed (item not
   present) or zero otherwise.  */
int
xg_lr0state_add_item (xg_lr0state *state, unsigned int prod, unsigned int dot)
{
  unsigned int i, n;
  xg_lr0item *it;

  /* Check for duplicates. */
  n = ulib_vector_length (&state->items);
  it = ulib_vector_front (&state->items);
  for (i = 0; i < n; ++i, ++it)
    if (it->prod == prod && it->dot == dot)
      return 0;

  /* Append the new item.  */
  if (ulib_vector_resize (&state->items, 1) == 0)
    {
      it = ulib_vector_back (&state->items);

      it [-1].prod = prod;
      it [-1].dot = dot;

      return 1;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to append an LR(0) item");
  return -1;
}

/* Return the number of items in the state.  */
unsigned int
xg_lr0state_count (const xg_lr0state *state)
{
  return ulib_vector_length (&state->items);
}

/* Return the Nth item in the state.  */
xg_lr0item *
xg_lr0state_get_item (const xg_lr0state *state, unsigned int n)
{
  return ulib_vector_elt (&state->items, n);
}

/* Sort the items in an LR(0) state.  */
static void
lr0state_sort (xg_lr0state *state)
{
  unsigned int i, n, tmp;
  xg_lr0item *a, *b;

  n = xg_lr0state_count (state);
  while (n--)
    {
      a = xg_lr0state_get_item (state, 0);
      for (i = 1; i < n; ++i)
        {
          b = xg_lr0state_get_item (state, i);

          if (a->dot < b->dot || (a->dot == b->dot && a->prod > b->prod))
            {
              tmp = a->dot;
              a->dot = b->dot;
              b->dot = tmp;

              tmp = a->prod;
              a->prod = b->prod;
              b->prod = tmp;
            }

          a = b;
        }
    }
}

/* Compute the closure of an LR(0) state.  */
static int
lr0state_closure (const xg_grammar *g, xg_lr0state *state, ulib_bitset *done)
{
  unsigned int i, j, n;
  xg_sym sym;
  xg_symdef *def;
  const xg_lr0item *it;
  const xg_prod *p;
 
  /* Get the next LR(0) item.  If the dot is in front of a terminal,
     skip the item, otherwise add to the state an item with the dot at
     the front for each production having the symbol as its left hand
     side.  Do not add duplicate items.  */
  for (i = 0; i < xg_lr0state_count (state); ++i)
    {
      /* Get next item, the production and the symbol following the
         dot.  */
      it = xg_lr0state_get_item (state, i);
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
        if (xg_lr0state_add_item (state, xg_symdef_get_prod (def, j), 0) < 0)
          return -1;
    }

  lr0state_sort (state);
  return 0;
}

/* Compute the closure of an LR(0) state.  */
int
xg_lr0state_closure (const xg_grammar *g, xg_lr0state *state)
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

  sts = lr0state_closure(g, state, &done);
  
  ulib_bitset_destroy (&done);
  
  return sts;
}

/* Compute the goto (STATE, SYM) function.  */
xg_lr0state *
xg_lr0state_goto (const xg_grammar *g, const xg_lr0state *src, xg_sym sym)
{
  unsigned int i, n;
  xg_lr0state *dst;
  xg_lr0item *it;
  xg_prod *p;

  if ((dst = xg_lr0state_new ()) == 0)
    return 0;

  n = xg_lr0state_count (src);
  for (i = 0; i < n; ++i)
    {
      it = xg_lr0state_get_item (src, i);
      p = xg_grammar_get_prod (g, it->prod);
      if (it->dot < xg_prod_length (p)
          && sym == xg_prod_get_symbol (p, it->dot))
        {
          if (xg_lr0state_add_item (dst, it->prod, it->dot + 1) < 0)
            return 0;
        }
    }

  if (xg_lr0state_closure (g, dst) < 0)
    return 0;

  return dst;
}

/* Display a debugging dump of an LR(0) state.  */
void
xg_lr0state_debug (FILE *out, const struct xg_grammar *g,
                  const xg_lr0state *state)
{
  unsigned int i, j, n, m;
  xg_prod *p;
  const xg_lr0item *it;

  n = xg_lr0state_count (state);
  for (i = 0; i < n; ++i)
    {
      it = xg_lr0state_get_item (state, i);
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


/* Initialize LR(0) states memory management.  */
int
xg__init_lr0states ()
{
  lr0state_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_lr0state),
                                      ULIB_CACHE_ALIGN, sizeof (void *),
                                      ULIB_CACHE_CTOR, lr0state_ctor,
                                      ULIB_CACHE_CLEAR, lr0state_clear,
                                      ULIB_CACHE_DTOR, lr0state_dtor,
                                      ULIB_CACHE_GC,
                                      0);
  if (lr0state_cache)
    return 0;
  else
    {
      ulib_log_printf (xg_log,
                       "ERROR: Unable to create the LR(0) states cache");
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
