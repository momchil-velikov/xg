/* grammar.c - grammar structures utility functions
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
#include <stdlib.h>

/* Create a symbol definition.  */
xg_symbol_def *
xg_symbol_new (char *name)
{
  xg_symbol_def *def;

  def = malloc (sizeof (xg_symbol_def));
  if (def)
    {
      def->code = 0;
      def->name = name;
      def->terminal = 0;
    }
  return def;
}

/* Delete a symbol definition.  */
void
xg_symbol_del (xg_symbol_def *def)
{
  if (def->name)
    free (def->name);
  free (def);
}


/* Create a production.  */
xg_production *
xg_production_new (xg_symbol lhs)
{
  xg_production *prod;

  prod = malloc (sizeof (xg_production));
  if (prod)
    {
      prod->lhs = lhs;
      if (! ulib_vector_init (&prod->rhs,
                              ULIB_ELT_SIZE, sizeof (xg_symbol),
                              ULIB_GROWTH_SCALE, 2,
                              0))
        return prod;
      free (prod);
    }
  return prod;
}

/* Delete a production.  */
void
xg_production_del (xg_production *prod)
{
  ulib_vector_destroy (&prod->rhs);
  free (prod);
}

/* Append a symbol to the right hand side.  */
int
xg_production_add (xg_production *prod, xg_symbol sym)
{
  return ulib_vector_append (&prod->rhs, &sym);
}


/* Create an empty grammar structure.  */
xg_grammar *
xg_grammar_new ()
{
  xg_grammar *g;

  if ((g = malloc (sizeof (xg_grammar))) != 0)
    {
      g->start = 0;
      if (ulib_vector_init (&g->syms, ULIB_DATA_PTR_VECTOR, 0) == 0)
        {
          if (ulib_vector_resize (&g->syms, XG_TOKEN_LITERAL_MAX + 1) == 0)
            {
              if (ulib_vector_init (&g->prods, ULIB_DATA_PTR_VECTOR, 0) == 0)
                return g;
            }
          ulib_vector_destroy (&g->syms);
        }
      free (g);
    }
  return 0;
}

/* Delete a grammar structure.  */
void
xg_grammar_del (xg_grammar *g)
{
  unsigned int i, n;

  /* Delete symbol definitions.  */
  n = ulib_vector_length (&g->syms);
  for (i = 0; i < n; ++i)
    xg_symbol_del ((xg_symbol_def *) ulib_vector_ptr_elt (&g->syms, i));

  /* Delete the productions.  */
  n = ulib_vector_length (&g->prods);
  for (i = 0; i < n; ++i)
    {
      xg_production *p = ulib_vector_ptr_elt (&g->prods, i);
      if (p)
        xg_production_del (p);
    }

  free (g);
}

/* Add a production to the grammar.  */
int
xg_grammar_add_production (xg_grammar *g, xg_production *p)
{
  return ulib_vector_append_ptr (&g->prods, p);
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: e85aaa11-d93a-414f-8851-eaca42e8404e
 * End:
 */
