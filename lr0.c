/* lr0.c - LR(0) DFA operation
 *
 * Copyright (C) 2005, 2006 Momchil Velikov
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
  (void) ulib_vector_init (&state->items,
                           ULIB_ELT_SIZE, sizeof (xg_lr0item), 0);
  (void) ulib_vector_init (&state->tr, ULIB_ELT_SIZE, sizeof (unsigned int), 0);
  (void) ulib_vector_init (&state->axns, ULIB_ELT_SIZE, sizeof (xg_lr0axn), 0);
  return 0;
}

/* LR(0) state clear.  */
static void
lr0state_clear (xg_lr0state *state, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_set_size (&state->items, 0);
  ulib_vector_set_size (&state->tr, 0);
  ulib_vector_set_size (&state->axns, 0);
}

/* LR(0) state destructor.  */
static void
lr0state_dtor (xg_lr0state *state, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_destroy (&state->items);
  ulib_vector_destroy (&state->tr);
  ulib_vector_destroy (&state->axns);
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

      it[-1].prod = prod;
      it[-1].dot = dot;

      return 1;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to append an LR(0) item");
  return -1;
}

/* Return the number of items in the state.  */
unsigned int
xg_lr0state_item_count (const xg_lr0state *state)
{
  return ulib_vector_length (&state->items);
}

/* Return the Nth item in the state.  */
xg_lr0item *
xg_lr0state_get_item (const xg_lr0state *state, unsigned int n)
{
  return ulib_vector_elt (&state->items, n);
}

/* Return a pointer to an array of LR(0) items.  The pointer is
   possibly invalidated after adding an item to the set.  */
xg_lr0item *
xg_lr0state_items_front (const xg_lr0state *state)
{
  return ulib_vector_front (&state->items);
}

/* Return a pointer just after last LR(0) item.  The pointer is
   possibly invalidated after adding an item to the set.  */
xg_lr0item *
xg_lr0state_items_back (const xg_lr0state *state)
{
  return ulib_vector_back (&state->items);
}


/* Add a transition to an LR(0) state.  */
int
xg_lr0state_add_trans (xg_lr0state *state, unsigned int no)
{
  if (ulib_vector_append (&state->tr, &no) < 0)
    {
      ulib_log_printf (xg_log,
                       "ERROR: Unable to append an LR(0) DFA transition");
      return -1;
    }

  return 0;
}

/* Get the number of transitions.  */
 unsigned int
xg_lr0state_trans_count (const xg_lr0state *state)
{
  return ulib_vector_length (&state->tr);
}

/* Get the Nth transition.  */
unsigned int
xg_lr0state_get_trans (const xg_lr0state *state, unsigned int n)
{
  return *(unsigned int *) ulib_vector_elt (&state->tr, n);
}


/* Add a parse action to an LR(0) state.  */
int
xg_lr0state_add_axn (xg_lr0state *state, xg_sym sym, unsigned int shift,
                     unsigned int no)
{
  xg_lr0axn *begin, *axn, *end;

  /* Find whether there are already other actions on SYM.  */
  begin = ulib_vector_front (&state->axns);
  end = ulib_vector_back (&state->axns);
  for (axn = begin; axn < end; ++axn)
    {
      if (axn->sym == sym)
        {
          /* Lookahead symbol found, insert the action so actions on
             the same lookaheads are adjiacent.  */
          xg_lr0axn act = { sym, shift, no };
          if (ulib_vector_insert (&state->axns, axn - begin, &act) < 0)
            goto error;
          return 0;
        }
    }

  /* First action on this lookahead.  */
  if (ulib_vector_resize (&state->axns, 1) < 0)
    goto error;

  axn = ulib_vector_back (&state->axns);

  axn [-1].sym = sym;
  axn [-1].shift = shift;
  axn [-1].no = no;

  return 0;

error:
  ulib_log_printf (xg_log, "ERROR: Unable to append a parse action.");
  return -1;
}

/* Get parse action count.  */
unsigned int
xg_lr0state_axn_count (const xg_lr0state *state)
{
  return ulib_vector_length (&state->axns);
}

/* Get a parse action.  */
const xg_lr0axn *
xg_lr0state_get_axn (const xg_lr0state *state, unsigned int n)
{
  return ulib_vector_elt (&state->axns, n);
}

/* Sort the items in an LR(0) state.  */
static void
lr0state_sort (xg_lr0state *state)
{
  unsigned int i, n, tmp;
  xg_lr0item *a, *b;

  n = xg_lr0state_item_count (state);
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
  for (i = 0; i < xg_lr0state_item_count (state); ++i)
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

  (void) ulib_bitset_init (&done);
  sts = lr0state_closure (g, state, &done);
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

  n = xg_lr0state_item_count (src);
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

/* Compare two LR(0) states for equality.  */
static int
lr0set_equal (const xg_lr0state *a, const xg_lr0state *b)
{
  unsigned int i, n;
  xg_lr0item *ai, *bi;

  /* Sets are not equal if their sizes differ.  */
  n = xg_lr0state_item_count (a);
  if (n != xg_lr0state_item_count (b))
    return 0;

  /* Compare each item.  Note that the states are sorted, thus first
     item difference indicates states differ also.  */
  for (i = 0; i < n; ++i)
    {
      ai = xg_lr0state_get_item (a, i);
      bi = xg_lr0state_get_item (b, i);

      if (ai->prod != bi->prod || ai->dot != bi->dot)
        return 0;
    }

  return 1;
}

/* Display a debugging dump of an LR(0) state.  */
void
xg_lr0state_debug (FILE *out, const struct xg_grammar *g,
                   const xg_lr0state *state)
{
  unsigned int i, j, n, m;
  xg_prod *p;
  const xg_lr0item *it;
  const xg_lr0axn *axn;

  /* Dump items.  */
  n = xg_lr0state_item_count (state);
  for (i = 0; i < n; ++i)
    {
      it = xg_lr0state_get_item (state, i);
      p = xg_grammar_get_prod (g, it->prod);

      fprintf (out, "\t%-4u: ", it->prod);
      xg_symbol_name_debug (out, g, p->lhs);
      fputs (" ->", out);

      for (j = 0; j < it->dot; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_prod_get_symbol (p, j));
        }

      fputs (" .", out);

      m = xg_prod_length (p);
      for (; j < m; ++j)
        {
          fputc (' ', out);
          xg_symbol_name_debug (out, g, xg_prod_get_symbol (p, j));
        }

      fputc ('\n', out);
    }

  /* Dump actions.  */
  fputc ('\n', out);
  n = xg_lr0state_axn_count (state);
  for (i = 0; i < n; ++i)
    {
      axn = xg_lr0state_get_axn (state, i);

      fputs ("\tOn ", out);
      xg_symbol_name_debug (out, g, axn->sym);

      if (axn->shift)
        fprintf (out, " shift and go to state %u\n", axn->no);
      else
        fprintf (out, " reduce by production %u\n", axn->no);
    }
}


/* Create the LR(0) DFA.  */
static int
lr0dfa_create (const xg_grammar *g, xg_lr0dfa *dfa)
{
  int no, sts = -1;
  unsigned int i;
  xg_lr0state *src, *dst;
  const xg_lr0item *it, *end;
  const xg_prod *p;
  xg_sym sym;
  ulib_bitset trans_done, closure_done;

  /* Start at the closure of the LR(0) item <0, 0>.  */
  if ((src = xg_lr0state_new ()) == 0
      || xg_lr0state_add_item (src, 0, 0) < 0
      || xg_lr0state_closure (g, src) < 0
      || xg_lr0dfa_add_state (dfa, src) < 0)
    return -1;

  /* Initialize done sets.  */
  (void) ulib_bitset_init (&trans_done);
  (void) ulib_bitset_init (&closure_done);

  /* Walk over unprocessed states.  */
  for (i = 0; i < ulib_vector_length (&dfa->states); ++i)
    {
      src = ulib_vector_ptr_elt (&dfa->states, i);

      /* Walk over the items and compute each possible transition.  */
      it = xg_lr0state_items_front (src);
      end = xg_lr0state_items_back (src);
      while (it < end)
        {
          p = xg_grammar_get_prod (g, it->prod);
          if (it->dot < xg_prod_length (p))
            {
              sym = xg_prod_get_symbol (p, it->dot);
              if (ulib_bitset_is_set (&trans_done, sym) == 0)
                {
                  /* Compute the transition on SYM.  */
                  if (ulib_bitset_set (&trans_done, sym) < 0
                      || (dst = xg_lr0state_goto (g, src, sym)) == 0
                      || (no = xg_lr0dfa_add_state (dfa, dst)) < 0
                      || (no = xg_lr0dfa_add_trans (dfa, sym, src->id, no)) < 0
                      || xg_lr0state_add_trans (src, no) < 0)
                    goto exit;
                }
            }
          ++it;
        }
      ulib_bitset_clear_all (&trans_done);
    }

  sts = 0;

exit:
  ulib_bitset_destroy (&closure_done);
  ulib_bitset_destroy (&trans_done);
  return sts;
}

/* LR(0) DFA pointer scan function.  */
static int
lr0dfa_gcscan (xg_lr0dfa *dfa, void **ptr, unsigned int sz)
{
  unsigned int i, n;
  xg_lr0state **state;

  n = ulib_vector_length (&dfa->states);
  if (n > sz)
    return -n;

  state = ulib_vector_front (&dfa->states);
  for (i = 0; i < n; ++i)
    *ptr++ = *state++;

  return n;
}

/* Create an LR(0) DFA.  */
xg_lr0dfa *
xg_lr0dfa_new (const xg_grammar *g)
{
  xg_lr0dfa *dfa;

  if ((dfa = malloc (sizeof (xg_lr0dfa))) != 0)
    {
      (void) ulib_vector_init (&dfa->states, ULIB_DATA_PTR_VECTOR, 0);
      (void) ulib_vector_init (&dfa->trans,
                               ULIB_ELT_SIZE, sizeof (xg_lr0trans), 0);
      if (lr0dfa_create (g, dfa) == 0)
        {
          if (ulib_gcroot (dfa, (ulib_gcscan_func) lr0dfa_gcscan) == 0)
            return dfa;
        }
      ulib_vector_destroy (&dfa->trans);
      ulib_vector_destroy (&dfa->states);
      free (dfa);
    }

  ulib_log_printf (xg_log, "ERROR: Out of memory creating LR(0) DFA");
  return 0;
}

/* Delete an LR(0) DFA.  */
void
xg_lr0dfa_del (xg_lr0dfa *dfa)
{
  ulib_gcunroot (dfa);
  ulib_vector_destroy (&dfa->states);
  ulib_vector_destroy (&dfa->trans);
  free (dfa);
}

/* Add a state to an LR(0) DFA.  Return an index of an LR(0) DFA
   state, which may not refer to S, if a state, which compares equal
   to S, already exists.  Return negative on error.  */
int
xg_lr0dfa_add_state (xg_lr0dfa *dfa, xg_lr0state *s)
{
  unsigned int n;
  xg_lr0state **old;

  n = ulib_vector_length (&dfa->states);
  old = ulib_vector_front (&dfa->states);
  while (n--)
    {
      if (lr0set_equal (*old, s))
        return old - (xg_lr0state **) ulib_vector_front (&dfa->states);
      ++old;
    }

  if (ulib_vector_append_ptr (&dfa->states, s) < 0)
    return -1;
  else
    {
      s->id = ulib_vector_length (&dfa->states) - 1;
      return s->id;
    }
}

/* Get the number of LR(0) DFA states.  */
unsigned int
xg_lr0dfa_state_count (const xg_lr0dfa *dfa)
{
  return ulib_vector_length (&dfa->states);
}

/* Get the N-th  LR(0) DFA state.  */
xg_lr0state *
xg_lr0dfa_get_state (const xg_lr0dfa *dfa, unsigned int n)
{
  return ulib_vector_ptr_elt (&dfa->states, n);
}

/* Add a transition to DST on symbol SYM to the LR (0) DFA.  */
int
xg_lr0dfa_add_trans (xg_lr0dfa *dfa, xg_sym sym, unsigned int src,
                     unsigned int dst)
{
  xg_lr0trans *t;

  if (ulib_vector_resize (&dfa->trans, 1) == 0)
    {
      t = ulib_vector_back (&dfa->trans);

      t [-1].id = ulib_vector_length (&dfa->trans) - 1;
      t [-1].sym = sym;
      t [-1].src = src;
      t [-1].dst = dst;

      return t [-1].id;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to create an LR(0) DFA transition");
  return -1;
}

/* Get the number of LR(0) DFA transitions.  */
unsigned int
xg_lr0dfa_trans_count (const xg_lr0dfa *dfa)
{
  return ulib_vector_length (&dfa->trans);
}

/* Get the N-th  LR(0) DFA transition.  */
xg_lr0trans *
xg_lr0dfa_get_trans (const xg_lr0dfa *dfa, unsigned int n)
{
  return ulib_vector_elt (&dfa->trans, n);
}


/* Create shift actions for all LR parsers.  */
static int
create_shift_actions (const xg_lr0dfa *dfa, xg_lr0state *state)
{
  unsigned int i, n, id;
  const xg_lr0trans *t;

  n = xg_lr0state_trans_count (state);
  for (i = 0; i < n; ++i)
    {
      id = xg_lr0state_get_trans (state, i);
      t = xg_lr0dfa_get_trans (dfa, id);

      if (xg_lr0state_add_axn (state, t->sym, 1, t->dst) < 0)
        return -1;
    }

  return 0;
}

/* Create actions for an SLR(1) parser.  */
int
xg_lr0dfa_make_slr_actions (const xg_grammar *g, xg_lr0dfa *dfa)
{
  unsigned int i, j, n, m;
  xg_lr0state *state;
  const xg_lr0item *it, *end;
  const xg_prod *p;
  const xg_symdef *def;

  n = xg_lr0dfa_state_count (dfa);

  for (i = 0; i < n; ++i)
    {
      state = ulib_vector_ptr_elt (&dfa->states, i);

      /* Create shift actions.  */
      if (create_shift_actions (dfa, state) < 0)
        return -1;

      /* Walk over the final items and create each possible SLR(1)
         reduction.  */
      it = xg_lr0state_items_front (state);
      end = xg_lr0state_items_back (state);
      while (it < end)
        {
          p = xg_grammar_get_prod (g, it->prod);
          if (it->dot == xg_prod_length (p))
            {
              def = xg_grammar_get_symbol (g, p->lhs);

              m = ulib_bitset_max (&def->follow);
              for (j = 0; j < m; ++j)
                {
                  if (ulib_bitset_is_set (&def->follow, j)
                      && xg_lr0state_add_axn (state, j, 0, it->prod) < 0)
                    return -1;
                }
            }
          ++it;
        }
    }

  return 0;
}


/* Display a debugging dump of an LR(0) DFA.  */
void
xg_lr0dfa_debug (FILE *out, const xg_grammar *g, const xg_lr0dfa *dfa)
{
  unsigned int i, n;
  xg_lr0state *state;

  fputs ("LR(0) DFA:\n", out);
  fputs ("==========\n\n", out);

  n = xg_lr0dfa_state_count (dfa);
  for (i = 0; i < n; ++i)
    {
      state = xg_lr0dfa_get_state (dfa, i);
      fprintf (out, "State %u:\n", i);
      xg_lr0state_debug (out, g, state);
      fputc ('\n', out);
    }
}


/* Initialize LR(0) DFA memory management.  */
int
xg__init_lr0dfa ()
{
  lr0state_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_lr0state),
                                      ULIB_CACHE_ALIGN, sizeof (void *),
                                      ULIB_CACHE_CTOR, lr0state_ctor,
                                      ULIB_CACHE_CLEAR, lr0state_clear,
                                      ULIB_CACHE_DTOR, lr0state_dtor,
                                      ULIB_CACHE_GC, 0);
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
