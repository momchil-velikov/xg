/* random-gen.c - generate a random sentence
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
#include <stdio.h>
#include <time.h>
#include <ulib/rand.h>

/* Produce a derivation from the symbol S with recursion budget REC.
   Append derived terminal symbols to V.  */
static int
expand (ulib_vector *v, const xg_grammar *g, xg_sym s, unsigned int rec)
{
  unsigned int slen, plen, r, n, *order;
  xg_symdef *def;
  xg_sym *syms;
  xg_prod *p;
  
  slen = ulib_vector_length (v);

  if (xg_grammar_is_terminal_sym (g, s))
    /* Emit a terminal symbol.  */
    return ulib_vector_set (v, slen, &s);

  /* Check whether recursion budget is exceeded.  */
  if (rec == 0)
    return -1;

  def = xg_grammar_get_symbol (g, s);
  n = xg_symdef_prod_count (def);

  /* Generate a random order of possible derivations.  */
  if ((order = malloc (n * sizeof (unsigned int))) == 0)
    return -1;
  for (r = 0; r < n; ++r)
    order [r] = r;
  ulib_shuffle (n, order);

  /* Find an alternative, capable of deriving a terminal string within
     the budget.  */
  for (r = 0; r < n; ++r)
    {
      (void) ulib_vector_set_size (v, slen);

      p = xg_grammar_get_prod (g, xg_symdef_get_prod (def, order [r]));
      plen = xg_prod_length (p);
      syms = xg_prod_get_symbols (p);
      while (plen)
        {
          if (expand (v, g, *syms++, rec - 1) < 0)
            break;
          plen--;
        }

      if (plen == 0)
        {
          free (order);
          return 0;
        }
    }

  free (order);
  return -1;
}

/* Output to OUT a random sentence from the language, defined by the
   grammar G.  The parameter SIZE indirectly influences the length of
   the generated sentence.  */
int
xg_make_random_sentence (FILE *out, const xg_grammar *g, unsigned int size)
{
  ulib_vector v;
  xg_sym *syms;
  unsigned int len;
  xg_symdef *def;

  /* Generate the sentence.  */
  (void) ulib_vector_init (&v, ULIB_ELT_SIZE, sizeof (unsigned int), 0);
  srand (time (0));
  if (expand (&v, g, g->start, size) < 0)
    return -1;

  /* Output the sentence.  */
  len = ulib_vector_length (&v);
  syms = ulib_vector_front (&v);
  while (len--)
    {
      if (*syms < XG_TOKEN_LITERAL_MAX)
        fprintf (out, "%c ", *syms);
      else
        {
          def = xg_grammar_get_symbol (g, *syms);
          fprintf (out, "%s ", def->name);
        }
      ++syms;
    }
  fputc ('\n', out);

  ulib_vector_destroy (&v);
  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 52d0f852-95ea-4238-b901-5c575fa6635c
 * End:
 */
