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
xg_make_symbol (char *name)
{
  xg_symbol_def *def;

  def = malloc (sizeof (xg_symbol_def));
  if (def)
    def->name = name;
  return def;
}

/* Create a production.  */
xg_production *
xg_make_production (xg_symbol lhs)
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

/* Append a symbol to the right hand side.  */
int
xg_production_add (xg_production *prod, xg_symbol sym)
{
  return ulib_vector_append (&prod->rhs, &sym);
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: e85aaa11-d93a-414f-8851-eaca42e8404e
 * End:
 */
