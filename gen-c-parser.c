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
 * along with XG; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */

#include "grammar.h"
#include "lr0.h"
#include <stdio.h>

int
xg_gen_c_parser (FILE *out, const xg_grammar *g, const xg_lr0dfa *dfa)
{
  xg_sym sym, k;
  unsigned int i, j, n, m;
  const xg_lr0state *state;
  const xg_lr0trans *tr;
  const xg_lr0reduct *rd;
  const xg_prod *p;
  const xg_symdef *def;

  /* Include the common parser declarations.  */
  fputs ("#include <xg-c-parser.h>\n\n", out);

  /* Emit symbol names.  */
  fputs ("#ifndef NDEBUG\n", out);
  fputs ("static const char *xg__symbol_name [] =\n"
         "{\n",
         out);
  n = xg_grammar_symbol_count (g);
  for (i = XG_TOKEN_LITERAL_MAX + 1; i < n; ++i)
    {
      def = xg_grammar_get_symbol (g, i);
      fprintf (out, "  \"%s\",\n",  def->name);
    }
  fputs ("  0\n};\n\n", out);

  /* Emit productions.  */
  fputs ("static const char *xg__prod [] =\n"
         "{\n",
         out);
  n = xg_grammar_prod_count (g);
  for (i = 0; i < n; ++i)
    {
      p = xg_grammar_get_prod (g, i);
      fputs ("  \"", out);
      xg_prod_print (out, g, p);
      fputs ("\",\n", out);
    }
  fputs ("  0\n};\n\n", out);
  fputs ("#endif /* NDEBUG */\n\n", out);

  /* Emit parser function preambule.  */
  fputs ("int\n"
         "xg_parse (xg_parse_ctx *ctx)\n"
         "{\n"
         "  XG__PARSER_FUNCTION_START;\n\n",
         out);

  /* Emit parse actions for each state.  */
  n = xg_lr0dfa_state_count (dfa);
  for (i = 0; i < n; ++i)
    {
      /* Emit stack manipulation.  */
      state = xg_lr0dfa_get_state (dfa, i);
      fprintf (out,
               "shift_%u:\n"
               "  XG__SHIFT;\n"
               "push_%u:\n"
               "  XG__PUSH (%u);\n\n",
               i, i, i);

      /* Emit shift actions.  */
      m = xg_lr0state_trans_count (state);
      fputs ("  switch (token)\n"
             "    {\n",
             out);
      for (j = 0; j < m; ++j)
        {
          tr = xg_lr0dfa_get_trans (dfa, xg_lr0state_get_trans (state, j));
          if (xg_grammar_is_terminal_sym (g, tr->sym))
            fprintf (out,
                     "    case %u:\n"
                     "      goto shift_%u;\n",
                     tr->sym, tr->dst);
        }
      fputs ("    }\n\n", out);

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
          fputs ("  switch (token)\n"
                 "    {\n",
                 out);
          for (j = 0; j < m; ++j)
            {
              rd = xg_lr0state_get_reduct (state, j);
            
              k = ulib_bitset_max (&rd->la);
              for (sym = 0; sym < k; ++sym)
                if (ulib_bitset_is_set (&rd->la, sym))
                  fprintf (out,
                           "    case %u:\n"
                           "      goto reduce_%u;\n",
                           sym, rd->prod);
            }
          fputs ("    default:\n"
                 "      goto parse_error;\n"
                 "    }\n\n\n",
                 out);
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
               "  XG__REDUCE (%u, %u);\n"
               "  goto symbol_%u;\n\n",
               i, i, xg_prod_length (p), p->lhs);
    }

  /* Emit non-terminal transitions code.  For each non-terminal
     symbol, jump to the appropriate destination state, depending on
     the current top of the stack state.  */
  k = xg_grammar_symbol_count (g);
  m = xg_lr0dfa_trans_count (dfa);
  for (sym = XG_TOKEN_LITERAL_MAX + 1; sym < k; ++sym)
    {
      if (xg_grammar_is_terminal_sym (g, sym))
        continue;

      fprintf (out, "symbol_%u:\n", sym);
      fputs ("  switch (state)\n"
             "    {\n",
             out);
      for (j = 0; j < m; ++j)
        {
          tr = xg_lr0dfa_get_trans (dfa, j);
          if (tr->sym == sym)
            {
              fprintf (out,
                       "    case %u:\n"
                       "      goto push_%u;\n",
                       tr->src, tr->dst);
            }
        }
      fputs ("    default:\n"
             "      goto internal_error;\n",
             out);
      fputs ("    }\n\n", out);
    }

  fputs ("internal_error:\n"
         "  XG__PARSER_FUNCTION_END (-1);\n\n",
         out);
  fputs ("parse_error:\n"
         "  XG__PARSER_FUNCTION_END (-1);\n\n",
         out);
  fputs ("accept:\n"
         "  XG__PARSER_FUNCTION_END (0);\n",
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

