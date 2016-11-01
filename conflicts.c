/* conflicts.c - resolve parsing action conflicts
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
 * along with XG; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */
#include "grammar.h"
#include "lr0.h"
#include "xg.h"
#include <ulib/log.h>

enum xg_resolution
{
  xg_resolve_none,
  xg_resolve_shift,
  xg_resolve_shift_default,
  xg_resolve_reduce,
  xg_resolve_error
};

/* Log a shift/reduce conflict resolution.  */
static void
log_shift_reduce_conflict (unsigned int state, const xg_symdef *la,
                           const xg_lr0reduct *rd, enum xg_resolution res)
{
  const char *name;
  char nm [2];

  if (la->name)
    name = la->name;
  else
    {
      name = nm;
      nm [0] = la->code;
      nm [1] = '\0';
    }

  ulib_log_printf (xg_log,
                   "State %u: shift/reduce conflict between symbol ``%s''",
                   state, name);

  ulib_log_printf (xg_log,
                   "State %u:   and production %u resolved as %s\n",
                   state, rd->prod,
                   res == xg_resolve_shift || res == xg_resolve_shift_default
                   ? "shift"
                   : res == xg_resolve_reduce
                   ? "reduce" : "error");
}

/* Log a reduce/reduce conflict resolution.  */

static void
log_reduce_reduce_conflict (unsigned int state, const xg_symdef *la,
                            unsigned int p1, unsigned int p2, unsigned int res)
{
  const char *name;
  char nm [2];

  if (la->name)
    name = la->name;
  else
    {
      name = nm;
      nm [0] = la->code;
      nm [1] = '\0';
    }

  ulib_log_printf (xg_log,
                   "State %u: reduce/reduce conflict between productions"
                   " %u and %u",
                   state, p1, p2);
  ulib_log_printf (xg_log,
                   "State %u:   on lookahead ``%s'' resolved in favor of"
                   " production %u\n",
                   state, name, res);
}

/* Resolve a conflict in STATE between a shift on LA and a reduction
   by a production with rightmost terminal RM.  */
static enum xg_resolution
resolve_shift_reduce_conflict (const xg_symdef *la, const xg_symdef *rm)
{
  assert (la != 0);
  if (rm && rm->assoc != xg_assoc_unknown && la->assoc != xg_assoc_unknown)
    {
      /* Resolve conflict based on precedence and associativity.  */
      if (rm->prec > la->prec)
        return xg_resolve_reduce;
      else if (rm->prec < la->prec)
        return xg_resolve_shift;
      else
        {
          /* Same precedence.  */
          if (rm->assoc == xg_assoc_left)
            return xg_resolve_reduce;
          else if (rm->assoc == xg_assoc_none && rm->code == la->code)
            return xg_resolve_error;
          else
            return xg_resolve_shift;
        }
    }
  else
    /* Resolve as shift according to the maximum munch rule.  */
    return xg_resolve_shift_default;
}

/* Resolve all shift/reduce conflicts in STATE.  */
static void
resolve_shift_reduce_conflicts (const xg_grammar *g, xg_lr0dfa *dfa,
                                xg_lr0state *state)
{
  unsigned int trno;
  unsigned int nrd, rdno;
  xg_lr0trans *tr;
  xg_lr0reduct *rd;
  const xg_prod *p;
  const xg_symdef *la, *rm;
  enum xg_resolution r;

  nrd = xg_lr0state_reduct_count (state);
  assert (nrd != 0);

  trno = 0;
  while (trno < xg_lr0state_trans_count (state))
    {
      tr = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (state, trno));

      /* Skip non-terminal transitions.  */
      if (! xg_grammar_is_terminal_sym (g, tr->sym))
        {
          ++trno;
          continue;
        }

      assert (tr->sym != XG_EPSILON);

      la = xg_grammar_get_symbol (g, tr->sym);

      /* Walk over reductions.  */
      for (rdno = 0; rdno < nrd; ++rdno)
        {
          r = xg_resolve_none;
          rd = xg_lr0state_get_reduct (state, rdno);

          if (! ulib_bitset_is_set (&rd->la, la->code))
            continue;

          /* If the current transition symbol is present as a
             lookahead for this reduction, we have a conflict to
             resolve.  */
          p = xg_grammar_get_prod (g, rd->prod);
          rm = xg_grammar_get_symbol (g, p->prec);

          r = resolve_shift_reduce_conflict (la, rm);
          if (r == xg_resolve_shift_default || r == xg_resolve_error)
            log_shift_reduce_conflict (state->id, la, rd, r);

          if (r == xg_resolve_shift || r == xg_resolve_shift_default)
            ulib_bitset_clear (&rd->la, la->code);
          else if (r == xg_resolve_reduce)
            {
              xg_lr0state_del_trans (state, trno);
              break;
            }
          else if (r == xg_resolve_error)
            {
              ulib_bitset_clear (&rd->la, la->code);
              xg_lr0state_del_trans (state, trno);
              break;
            }
        }

      if (r == xg_resolve_none
          || r == xg_resolve_shift
          || r == xg_resolve_shift_default)
        ++trno;
    }
}

/* Resolve all reduce/reduce conflicts in STATE.  */
static void
resolve_reduce_reduce_conflicts (const xg_grammar *g, xg_lr0state *state)
{
  unsigned int i, j, n;
  unsigned int b, bmax;
  xg_lr0reduct *rdi, *rdj;
  const xg_symdef *def;

  n = xg_lr0state_reduct_count (state);
  for (i = 0; i < n; ++i)
    {
      rdi = xg_lr0state_get_reduct (state, i);
      bmax = ulib_bitset_max (&rdi->la);

      for (j = i + 1; j < n; ++j)
        {
          rdj = xg_lr0state_get_reduct (state, j);

          for (b = 0; b < bmax; ++b)
            {
              if (ulib_bitset_is_set (&rdi->la, b)
                  && ulib_bitset_is_set (&rdj->la, b))
                {
                  def = xg_grammar_get_symbol (g, b);
                  if (rdi->prod < rdj->prod)
                    {
                      ulib_bitset_clear (&rdj->la, b);
                      log_reduce_reduce_conflict (state->id, def, rdi->prod,
                                                  rdj->prod, rdi->prod);
                    }
                  else
                    {
                      ulib_bitset_clear (&rdi->la, b);
                      log_reduce_reduce_conflict (state->id, def, rdi->prod,
                                                  rdj->prod, rdj->prod);
                    }
                }
            }
        }
    }
}

/* Resolve parsing conflicts.  */
int
xg_resolve_conflicts (const xg_grammar *g, xg_lr0dfa *dfa)
{
  unsigned int nstates, stateno;
  xg_lr0state *state;
  xg_lr0reduct *rd;
  unsigned int n;

  nstates = xg_lr0dfa_state_count (dfa);
  for (stateno = 0; stateno < nstates; ++stateno)
    {
      state = xg_lr0dfa_get_state (dfa, stateno);

      if ((n = xg_lr0state_reduct_count (state)) != 0)
        {
          /* Resolve the coflicts.  */
          resolve_shift_reduce_conflicts (g, dfa, state);
          resolve_reduce_reduce_conflicts (g, state);

          /* Remove reductions with empty lookahead sets.  */
          while (n--)
            {
              rd = xg_lr0state_get_reduct (state, n);
            
              if (ulib_bitset_is_empty (&rd->la))
                xg_lr0state_del_reduct (state, n);
            }
        }
    }

  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * End:
 */
