/* gen-c-parser.c - generate a parser in ISO C
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

#include "grammar.h"
#include "lr0.h"
#include <stdio.h>

int
xg_gen_c_parser (FILE *out, const xg_grammar *g, const xg_lr0dfa *dfa)
{
  int have_case;
  int last_dst;
  xg_sym sym, k;
  unsigned int i, j, n, m;
  const xg_lr0state *state;
  const xg_lr0trans *tr;
  const xg_lr0reduct *rd;
  const xg_prod *p;

  /* Include the common parser declarations.  */
  fputs ("#include <xg-c-parser.h>\n\n", out);

  /* Emit parser function preambule.  */
  fputs ("int\n"
         "xg_parse (xg_parse_ctx *ctx)\n"
         "{\n"
         "  int token;\n"
         "  void *value;\n"
         "\n"
         "  unsigned int lhs;\n"

         "\n"
         "#define SHIFT \\\n"
         "  do \\\n"
         "    { \\\n"
         "      xg__stack_top (&ctx->stk)->value = value;   \\\n"
         "      if ((token = ctx->get_token (&value)) == -1) \\\n"
         "        goto lexer_error; \\\n"
         "    } \\\n"
         "  while (0)"
         "\n"
         "#define PUSH(n) xg__stack_push (&ctx->stk, n)\n"
         "\n"
         "  if (xg__stack_init (&ctx->stk) < 0)\n"
         "    return -1;\n"
         "\n"
         "  if ((token = ctx->get_token (&value)) == -1)\n"
         "    goto lexer_error;\n"
         "  goto push_0;\n"
         "\n", out);

  /* Emit parse actions for each state.  */
  n = xg_lr0dfa_state_count (dfa);
  for (i = 0; i < n; ++i)
    {
      /* Emit stack manipulation.  */
      state = xg_lr0dfa_get_state (dfa, i);
      fprintf (out,
               "shift_%u:\n"
               "  SHIFT;\n"
               "push_%u:\n"
               "  PUSH (%u);\n\n",
               i, i, i);

      /* Emit shift actions.  */
      m = xg_lr0state_trans_count (state);
      for (j = 0; j < m; ++j)
        {
          tr = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (state, j));
          if (xg_grammar_is_terminal_sym (g, tr->sym))
            fprintf (out,
                     "  if (token == %u)\n"
                     "    goto shift_%u;\n",
                     tr->sym, tr->dst);
        }

      /* Emit reduce actions.  */
      m = xg_lr0state_reduct_count (state);
      if (m == 1)
        {
          /* If there's only one reduction, jump straight to the
             reduction code, without checking lookaheads.  The
             eventual error will be detected later, when we have to
             shift the errorneous token.  */
          rd = xg_lr0state_get_reduct (state, 0);
          fprintf (out, "  goto reduce_%u;\n\n\n", rd->prod);
        }
      else if (m == 0)
        {
          /* If there are no reductions, jump to the parse error
             handling code, unless this is the accepting state.  */
          if (state->accept)
            fputs ("  goto accept;\n\n\n", out);
          else
            fputs ("  goto parse_error;\n\n\n", out);
        }
      else
        {
          for (j = 0; j < m; ++j)
            {
              rd = xg_lr0state_get_reduct (state, j);
            
              k = ulib_bitset_max (&rd->la);
              for (sym = 0; sym < k; ++sym)
                if (ulib_bitset_is_set (&rd->la, sym))
                  fprintf (out,
                           "  if (token == %u)\n"
                           "    goto reduce_%u;\n",
                           sym, rd->prod);
            }

          fputs ("  goto parse_error;\n\n\n", out);
        }
    }

  /* Emit reduce actions for each production. Skip "reduce" by
     production 0 as this constitutes an accept and is handled
     elsewhere.  */
  n = xg_grammar_prod_count (g);
  for (i = 1; i < n; ++i)
    {
      p = xg_grammar_get_prod (g, i);
      fprintf (out,
               "reduce_%u:\n"
               "  xg__stack_pop (&ctx->stk, %u);\n"
               "  lhs = %u;\n"
               "  goto next;\n\n",
               i, xg_prod_length (p), p->lhs);
    }

  /* Emit non-terminal transitions for each state.  */
  fputs ("next:\n"
         "  switch (xg__stack_top (&ctx->stk)->state)\n"
         "    {\n",
         out);
  n = xg_lr0dfa_state_count (dfa);
  for (i = 0; i < n; ++i)
    {
      state = xg_lr0dfa_get_state (dfa, i);
      have_case = 0;
      last_dst = -1;
      sym = -1;

      m = xg_lr0state_trans_count (state);
      for (j = 0; j < m; ++j)
        {
          tr = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (state, j));
          if (! xg_grammar_is_terminal_sym (g, tr->sym))
            {
              if (last_dst != -1)
                {
                  if (!have_case)
                    {
                      fprintf (out, "    case %u:\n", i);
                      have_case = 1;
                    }

                  fprintf (out,
                           "      if (lhs == %u)\n"
                           "        goto push_%u;\n",
                           sym, last_dst);
                }
              
              last_dst = tr->dst;
              sym = tr->sym;
            }
        }

      if (last_dst != -1)
        {
          if (!have_case)
            fprintf (out, "    case %u:\n", i);
          
          fprintf (out, "      goto push_%u;\n\n", last_dst);
        }
    }
  fputs ("    }\n\n", out);

  fputs ("parse_error:\n"
         "  return -1;\n\n",
         out);
  fputs ("lexer_error:\n"
         "  return -1;\n\n",
         out);
  fputs ("accept:\n"
         "  return 0;\n",
         out);
  fputs ("}\n", out);

  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 4ae49fbc-f2ad-4060-b817-31d27994977e
 * End:
 */

