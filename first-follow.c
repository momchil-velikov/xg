/* first-follow.c - FIRST and FOLLOW functions for symbols and
 * sentenial forms.
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
#include "grammar.h"
#include "xg.h"

const ulib_bitset *xg_epsilon_set;
const ulib_bitset *xg_eof_set;

/* Compute the FIRST set for each non-terminal.  */
int
xg_grammar_compute_first (const xg_grammar *g)
{
  int chg, sts;
  unsigned int i, j, n, m;
  xg_sym sym;
  xg_prod *p;
  xg_symdef *ls, *rs;

  n = xg_grammar_prod_count (g);
  chg = 1;
  while (chg)
    {
      chg = 0;
      for (i = 0; i < n; ++i)
        {
          p = xg_grammar_get_prod (g, i);
          ls = xg_grammar_get_symbol (g, p->lhs);
          m = xg_prod_length (p);
          if (m == 0)
            {
              /* For each production X -> epsilon, add epsilon to
                 FIRST (X).  */
              chg = chg || ! ulib_bitset_is_set (&ls->first, XG_EPSILON);
              if (ulib_bitset_set (&ls->first, XG_EPSILON) < 0)
                goto error;
            }
          else
            {
              /* For each production X -> Y1 Y2 Y3 ... Yn, add to
                 FIRST(A) each symbol in FIRST (Yi) iff epsilon is in
                 each FIRST(Yi-1).  Add epsilon to FIRST (Y) iff
                 epsilon is in each FIRST (Yi).  */
              for (j = 0; j < m; ++j)
                {
                  sym = xg_prod_get_symbol (p, j);
                  if (xg_grammar_is_terminal_sym (g, sym))
                    {
                      if (! ulib_bitset_is_set (&ls->first, sym))
                        {
                          chg = 1;
                          if (ulib_bitset_set (&ls->first, sym) < 0)
                            goto error;
                        }
                      break;
                    }
                  else
                    {
                      rs = xg_grammar_get_symbol (g, sym);
                      if (ls != rs)
                        {
                          sts = ulib_bitset_destr_or_andn_chg (&ls->first,
                                                               &rs->first,
                                                               xg_epsilon_set);
                          if (sts < 0)
                            goto error;
                          chg = chg || (sts > 0);
                        }
                      if (! ulib_bitset_is_set (&rs->first, XG_EPSILON))
                        break;
                    }
                }

              if (j == m
                  && ulib_bitset_set (&ls->first, XG_EPSILON) < 0)
                goto error;
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log, "ERROR: Out of memory computing FIRST sets");
  return -1;
}

/* Compute the FOLLOW set for each non-terminal.  */
int
xg_grammar_compute_follow (const xg_grammar *g)
{
  int chg, sts;
  unsigned int i, j, k, n, m;
  xg_prod *p;
  xg_symdef *ls, *rs, *fs;
  xg_sym sym;

  /* Add the end-of-input marker to the FOLLOW set of the start
     symbol.  */
  ls = xg_grammar_get_symbol (g, g->start);
  if (ulib_bitset_set (&ls->follow, XG_EOF) < 0)
    goto error;

  n = xg_grammar_prod_count (g);
  chg = 1;
  while (chg)
    {
      chg = 0;
      for (i = 0; i < n; ++i)
        {
          /* For each production X -> aYc, add to FOLLOW(Y) all the
             symbols in FIRST(c), except epsilon. If c derives
             epsilon, add FOLLOW(X) to FOLLOW(Y).  */
          p = xg_grammar_get_prod (g, i);
          ls = xg_grammar_get_symbol (g, p->lhs);
          m = xg_prod_length (p);
          for (j = 0; j < m; ++j)
            {
              /* Update FOLLOW (RS).  */
              sym = xg_prod_get_symbol (p, j);
              if (xg_grammar_is_terminal_sym (g, sym))
                continue;
              rs = xg_grammar_get_symbol (g, sym);

              for (k = j + 1; k < m; ++k)
                {
                  sym = xg_prod_get_symbol (p, k);
                  if (xg_grammar_is_terminal_sym (g, sym))
                    {
                      /* Add the terminal SYM to FOLLOW (RS).  */
                      if (! ulib_bitset_is_set (&rs->follow, sym))
                        {
                          chg = 1;
                          if (ulib_bitset_set (&rs->follow, sym) < 0)
                            goto error;
                        }
                      break;
                    }
                  else
                    {
                      /* Add all the symbols in FIRST (FS) to FOLLOW
                         (RS).  */
                      fs = xg_grammar_get_symbol (g, sym);
                      sts = ulib_bitset_destr_or_andn_chg (&rs->follow,
                                                           &fs->first,
                                                           xg_epsilon_set);
                      if (sts < 0)
                        goto error;
                      chg = chg || (sts > 0);
                      if (! ulib_bitset_is_set (&fs->first, XG_EPSILON))
                        break;
                    }
                }

              if (k >= m && ls != rs)
                {
                  /* Add FOLLOW(LS) to FOLLOW(RS).  */
                  sts = ulib_bitset_destr_or_chg (&rs->follow, &ls->follow);
                  if (sts < 0)
                    goto error;
                  chg = chg || (sts > 0);
                }
            }
        }
    }

  return 0;

error:
  ulib_log_printf (xg_log, "ERROR: Out of memory computing FOLLOW sets");
  return -1;
}


/* Check whether the symbol S is nullable.  */
int
xg_nullable_sym (const xg_grammar *g, xg_sym s)
{
  xg_symdef *def;

  if (s == XG_EPSILON)
    return 1;

  if (xg_grammar_is_terminal_sym (g, s))
    return 0;

  def = xg_grammar_get_symbol (g, s);
  return ulib_bitset_is_set (&def->first, XG_EPSILON);
}

/* Check whether the sentenial form FORM can derive the empty string.
   The FIRST set is a prerequisite for calling this function.  */
int
xg_nullable_form (const xg_grammar *g, unsigned int n, const xg_sym *form)
{
  while (n--)
    {
      if (! xg_nullable_sym (g, *form))
        return 1;
      ++form;
    }
  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 03a2d870-d4f5-4f37-9d92-cb4a2381dd2a
 * End:
 */
