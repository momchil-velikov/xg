/* lalr.c - LALR(1) look-ahead computation.
 *
 * Copyright (C) 2006 Momchil Velikov
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

/* Compute the LALR(1) look-ahead sets according to the algorithm
   described in

   "Efficient Computation of LALR(1) Look-Ahead Sets"
   Frank DeRemer, Thomas Pennello
   October 1982
   ACM Transactions on Programming Languages and Systems (TOPLAS),
   Volume 4 Issue 4
 */

#include "lalr.h"
#include "xg.h"
#include <ulib/cache.h>

/* LALR(1) transitions cache.  */
static ulib_cache *lalr_cache;

/* LALR(1) transition constructor.  */
static int
trans_ctor (xg_lalr_trans *tr, unsigned int size __attribute__ ((unused)))
{
  (void) ulib_vector_init (&tr->rel, ULIB_ELT_SIZE, sizeof (unsigned int), 0);
  (void) ulib_bitset_init (&tr->value);
  return 0;
}

/* LALR(1) transition clear.  */
static void
trans_clear (xg_lalr_trans *tr, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_set_size (&tr->rel, 0);
  ulib_bitset_clear_all (&tr->value);
}

/* Relation destructor.  */
static void
trans_dtor (xg_lalr_trans *tr, unsigned int size __attribute__ ((unused)))
{
  ulib_vector_destroy (&tr->rel);
  ulib_bitset_destroy (&tr->value);
}


/* Create the LALR(1) transitions array.  */
static xg_lalr_trans *const *
create_lalr_transitions (const xg_grammar *g, const xg_lr0dfa *dfa)
{
  unsigned int i, n;
  const xg_lr0trans *t;
  xg_lalr_trans **trans;

  /* Allocate the LALR(1) transitions array.  */
  if ((trans = calloc (xg_lr0dfa_trans_count (dfa), sizeof (void *))) == 0)
    goto error;

  /* Walk over all the non-terminal transitions in the LR(0) DFA
     states and create an LALR(1) transition in the TRANS vector,
     corresponding to each LR(0) transition.  */

  n = xg_lr0dfa_trans_count (dfa);
  for (i = 0; i < n; ++i)
    {
      t = xg_lr0dfa_get_trans (dfa, i);
      if (! xg_grammar_is_terminal_sym (g, t->sym)
          && (trans [i] = ulib_cache_alloc (lalr_cache)) == 0)
        goto error;
    }

  return trans;

error:
  ulib_log_printf (xg_log,
                   "ERROR: Unable to create LALR(1) transitions");
  return 0;
}

/* Clear relation vectors.  */
static void
clear_rel (unsigned int n, xg_lalr_trans *const *trans)
{
  unsigned int i;

  for (i = 0; i < n; ++i)
    if (trans [i])
      ulib_vector_clear (&trans [i]->rel, 0);
}


/* Initialize transitions with the DirectRead sets:
   DR (p, A) = {t in T | p -A-> r -t->.} */
static int
compute_direct_read_sets (const xg_grammar *g, const xg_lr0dfa *dfa,
                          xg_lalr_trans *const *trans)
{
  unsigned int i, j, m, n;
  const xg_lr0state *s;
  const xg_lr0trans *t;

  n = xg_lr0dfa_trans_count (dfa);
  for (i = 0; i < n; ++i)
    {
      /* Follow non-terminal transitions.  */
      t = xg_lr0dfa_get_trans (dfa, i);
      if (! xg_grammar_is_terminal_sym (g, t->sym))
        {
          s = xg_lr0dfa_get_state (dfa, t->dst);

          /* Insert all terminals, which label outgoing transitions,
             in the TRANS [I] value.  */
          m = xg_lr0state_trans_count (s);
          for (j = 0; j < m; ++j)
            {
              t = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (s, j));
              if (xg_grammar_is_terminal_sym (g, t->sym)
                  && ulib_bitset_set (&trans [i]->value, t->sym) < 0)
                goto error;
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log, "ERROR: Unable to compute DR sets");
  return -1;
}

/* Compute the ``reads'' relation:
   (p, A) ``reads'' (r, C) iff p -A-> r -C->. and C=>* eps */
static int
compute_reads_relation (const xg_grammar *g, const xg_lr0dfa *dfa,
                        xg_lalr_trans *const *trans)
{
  unsigned int i, j, m, n;
  const xg_lr0state *s;
  const xg_lr0trans *t;

  n = xg_lr0dfa_trans_count (dfa);
  for (i = 0; i < n; ++i)
    {
      /* Follow non-terminal transitions.  */
      t = xg_lr0dfa_get_trans (dfa, i);
      if (! xg_grammar_is_terminal_sym (g, t->sym))
        {
          s = xg_lr0dfa_get_state (dfa, t->dst);

          /* Find transitions with nullable non-terminals. */
          m = xg_lr0state_trans_count (s);
          for (j = 0; j < m; ++j)
            {
              t = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (s, j));
              if (! xg_grammar_is_terminal_sym (g, t->sym)
                  && xg_nullable_sym (g, t->sym)
                  && ulib_vector_append (&trans [i]->rel, &t->id) < 0)
                goto error;
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log, "ERROR: Unable to compute ``reads'' relation");
  return -1;
}

/* Helper function to find in the state S the transition labeled
   SYM.  */
static xg_lr0trans *
find_trans (const xg_lr0dfa *dfa, const xg_lr0state *s, xg_sym sym)
{
  unsigned int i, n;
  xg_lr0trans *t;

  n = xg_lr0state_trans_count (s);
  for (i = 0; i < n; ++i)
    {
      t = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (s, i));
      if (t->sym == sym)
        return t;
    }

  return 0;
}

/* Compute the  ``includes'' relation:
   (p, A) ``includes'' (p', B) iff B->bAy, y =>* eps and p' -..b..-> p */
static int
compute_includes_relation (const xg_grammar *g, const xg_lr0dfa *dfa,
                           xg_lalr_trans *const *trans)
{
  unsigned int i, n, j, m, k;
  xg_lr0state *s;
  xg_lr0trans *t, *tt;
  const xg_sym *sym;
  const xg_prod *p;
  const xg_symdef *def;

  n = xg_lr0dfa_trans_count (dfa);
  for (i = 0; i < n; ++i)
    {
      if (trans [i] == 0)
        /* Skip non-terminal transitions.  */
        continue;

      t = xg_lr0dfa_get_trans (dfa, i);

      /* Process each production, whose left hand side is the symbol,
         labeling the above transition.  */
      def = xg_grammar_get_symbol (g, t->sym);
      m = xg_symdef_prod_count (def);
      for (j = 0; j < m; ++j)
        {
          p = xg_grammar_get_prod (g, xg_symdef_get_prod (def, j));

          /* Follow the path defined by the right hand side of the
             production.  */
          sym = xg_prod_get_symbols (p);
          k = xg_prod_length (p);
          s = xg_lr0dfa_get_state (dfa, t->src);
          while (k > 0)
            {
              tt = find_trans (dfa, s, *sym);
              assert (tt);
              
              if (! xg_grammar_is_terminal_sym (g, *sym)
                  && (k == 1 || xg_nullable_form (g, k - 1, sym + 1)))
                {
                  /* Found a state and a transition, such that
                     TT ``includes'' T  */
                  if (ulib_vector_append (&trans [tt->id]->rel, &t->id) < 0)
                    goto error;
                }
              
              s = xg_lr0dfa_get_state (dfa, tt->dst);
              ++sym;
              --k;
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log,
                   "ERROR: Unable to compute the ``includes'' relation");
  return -1;
}

/* Compute the lookahead sets:
   LA (q, A->w) = U{Follow (p, A) | p -..w..-> q} */
static int
compute_lookaheads (const xg_grammar *g, xg_lr0dfa *dfa,
                    xg_lalr_trans *const *trans)
{
  unsigned int stateno, nstates, nitems, nfins, plen;
  xg_lr0state *start, *end;
  xg_lr0trans *t;
  xg_lr0reduct *rd;
  const xg_lr0item *it, *fin;
  const xg_prod *p;
  xg_sym *sym;

  /* Walk over the non-kernel items in each LR(0) DFA state.  */
  nstates = xg_lr0dfa_state_count (dfa);
  for (stateno = 0; stateno < nstates; ++stateno)
    {
      start = xg_lr0dfa_get_state (dfa, stateno);

      nitems = xg_lr0state_item_count (start);
      it = xg_lr0state_items_front (start);
      for (;nitems--; ++it)
        {
          if (it->dot)
            /* Skip kernel items.  */
            continue;
          
          /* Trace the path, defined by the item production's right
             hand side, to the state, which contains the final item
             for the production.  */
          p = xg_grammar_get_prod (g, it->prod);

          plen = xg_prod_length (p);
          sym = xg_prod_get_symbols (p);
          end = start;
          while (plen--)
            {
              t = find_trans (dfa, end, *sym);
              end = xg_lr0dfa_get_state (dfa, t->dst);
              ++sym;
            }

          /* Find the final item for the production.  */
          nfins = xg_lr0state_item_count (end);
          fin = xg_lr0state_items_front (end);
          while (nfins && (it->prod != fin->prod
                           || fin->dot != xg_prod_length (p)))
            {
              ++fin;
              --nfins;
            }
          assert (nfins != 0);

          /* Found a state END and a final item FIN, corresponding to
             production P, such that
             (END, P) ``lookback'' (START, lhs(P))  */
          t = find_trans (dfa, start, p->lhs);
          if (t == 0)
            {
              /* Accepting state.  */
              if (xg_lr0state_add_reduct (end, it->prod) == 0)
                goto error;
            }
          else
            {
              if ((rd = xg_lr0state_add_reduct (end, it->prod)) == 0
                  || ulib_bitset_destr_or (&rd->la, &trans [t->id]->value) < 0)
                goto error;
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log,
                   "ERROR: Unable to compute the LALR(1) lookahead sets");
  return -1;
}


/* Context passed around in DIGRAPH computations.  */
struct digraph_ctx
{
  /* Number of LALR (1) transitions.  */
  unsigned int ntrans;

  /* All LALR(1) transitions (NTRANS elements).  */
  xg_lalr_trans *const *trans;

  /* Stack depth at the SCC root (NTRANS elements).  */
  unsigned int *root;

  /* Stack for SCC member candidates.  */
  ulib_vector stk;
};
typedef struct digraph_ctx digraph_ctx;

/* Recursive visit of the transitions in DFS order over either the
   ``reads'' or the ``includes'' relation.  */
static int
digraph_visit (digraph_ctx *ctx, unsigned int no)
{
  unsigned int n, *next;

  /* Push the transitions on the stack.  It's a candidate for an SCC
     root.  */
  if (ulib_vector_append (&ctx->stk, &no) < 0)
    return -1;

  /* Record the current stack depth.  */
  ctx->root [no] = ulib_vector_length (&ctx->stk);

  /* Traverse the sucessors of the current transition.  */
  n = ulib_vector_length (&ctx->trans [no]->rel);
  next = ulib_vector_front (&ctx->trans [no]->rel);
  while (n--)
    {
      if (ctx->root [*next] == 0 && digraph_visit (ctx, *next) < 0)
        return -1;

      if (ctx->root [*next] < ctx->root [no])
        {
          /* Current transition is a part of an SCC, whose root is
             deeper in the stack.  */
          ctx->root [no] = ctx->root [*next];
        }

      if (ulib_bitset_destr_or (&ctx->trans [no]->value,
                                &ctx->trans [*next]->value) < 0)
        return -1;

      ++next;
    }

  if (ctx->root [no] == ulib_vector_length (&ctx->stk))
    {
      /* Found an SCC, with the current transition being the root and
         the transitions up the stack being the SCC members.  Pop the
         entire SCC. */
      do
        {
          next = ulib_vector_back (&ctx->stk);
          n = next [-1];
          ulib_vector_remove_last (&ctx->stk);

          ctx->root [n] = ~0U;
          if (n != no
              && ulib_bitset_copy (&ctx->trans [n]->value,
                                   &ctx->trans [no]->value) < 0)
            return -1;
        }
      while (n != no);
    }

  return 0;
}

/* Compute the function F x = F'x U U{F'y | x R y} on the directed
   graph defined by the relation R.  */
static int
digraph (digraph_ctx *ctx)
{
  unsigned int i, n;

  n = ctx->ntrans;
  for (i = 0; i < n; ++i)
    if (ctx->root [i] == 0 && ctx->trans [i] != 0 && digraph_visit (ctx, i) < 0)
      return -1;

  return 0;
}


/* Create reductions for an LALR(1) parser.  */
int
xg_make_lalr_reductions (const xg_grammar *g, xg_lr0dfa *dfa)
{
  int sts = -1;
  xg_lalr_trans *const *trans;
  digraph_ctx ctx;

  if ((trans = create_lalr_transitions (g, dfa)))
    {
      ctx.ntrans = xg_lr0dfa_trans_count (dfa);
      ctx.trans = trans;
      (void) ulib_vector_init (&ctx.stk,
                               ULIB_ELT_SIZE, sizeof (unsigned int), 0);
      if ((ctx.root = calloc (ctx.ntrans, sizeof (unsigned int))))
        {
          /* Compute Read sets */
          if (compute_direct_read_sets (g, dfa, trans) == 0
              && compute_reads_relation (g, dfa, trans) == 0
              && digraph (&ctx) == 0)
            {
              /* Prepare for the next run.  */
              memset (ctx.root, 0, ctx.ntrans * sizeof (unsigned int));
              clear_rel (ctx.ntrans, trans);

              /* Compute Follow sets */
              if (compute_includes_relation (g, dfa, trans) == 0
                  && digraph (&ctx) == 0)
                {
                  /* Compute lookahead sets */
                  sts = compute_lookaheads (g, dfa, trans);
                }
            }
          free (ctx.root);
        }
      ulib_vector_destroy (&ctx.stk);
      free ((void*) trans);
    }
  return sts;
}


int
xg__init_lalr (void)
{
  lalr_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_lalr_trans),
                                  ULIB_CACHE_ALIGN, sizeof (void *),
                                  ULIB_CACHE_CTOR, trans_ctor,
                                  ULIB_CACHE_CLEAR, trans_clear,
                                  ULIB_CACHE_DTOR, trans_dtor,
                                  ULIB_CACHE_GC, 0);
  if (lalr_cache)
    return 0;


  ulib_log_printf (xg_log,
                   "ERROR: Unable to create the LALR(1) transitions cache");
  return -1;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 1929ef8d-8aeb-462f-9e81-72a5641bbfb6
 * End:
 */
