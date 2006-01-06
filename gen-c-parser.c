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
  xg_sym last_sym;
  int last_no;
  unsigned int i, j, n, m;
  xg_lr0state *state;
  const xg_lr0axn *axn;
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

      /* Emit parse actions.  */
      m = xg_lr0state_axn_count (state);
      for (j = 0; j < m; ++j)
        {
          axn = xg_lr0state_get_axn (state, j);
          if (xg_grammar_is_terminal_sym (g, axn->sym))
            {
              fprintf (out,
                       "  if (token == %u)\n"
                       "    goto %s_%u;\n",
                       axn->sym, (axn->shift ? "shift" : "reduce"), axn->no);
            }
        }
      fputs ("  goto parse_error;\n\n\n", out);
    }

  /* Reduce by production 0 is the accepting state.  */
  fputs ("reduce_0:\n"
         "  goto accept;\n\n",
         out);

  /* Emit reduce actions for each production.  */
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
      last_no = -1;

      m = xg_lr0state_axn_count (state);
      for (j = 0; j < m; ++j)
        {
          axn = xg_lr0state_get_axn (state, j);
          if (!xg_grammar_is_terminal_sym (g, axn->sym))
            {
              if (last_no != -1)
                {
                  if (!have_case)
                    {
                      fprintf (out, "    case %u:\n", i);
                      have_case = 1;
                    }

                  fprintf (out,
                           "      if (lhs == %u)\n"
                           "        goto push_%u;\n",
                           last_sym, last_no);
                }
              
              last_no = axn->no;
              last_sym = axn->sym;
            }
        }

      if (last_no != -1)
        {
          if (!have_case)
            fprintf (out, "    case %u:\n", i);
          
          fprintf (out, "      goto push_%u;\n\n", last_no);
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

