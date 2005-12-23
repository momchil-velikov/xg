/* lr0.h - LR(0) set declarations
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

#ifndef xg__lr0_h
#define xg__lr0_h 1

#include <ulib/defs.h>
#include <ulib/vector.h>
#include <stdio.h>

BEGIN_DECLS

/* An LR(0) item.  */
struct xg_lr0item
{
  /* Production number.  */
  unsigned int prod;

  /* Dot position.  */
  unsigned int dot;
};
typedef struct xg_lr0item xg_lr0item;

/* An LR(0) item set/State in the  DFA for viable prefixes.  */
struct xg_lr0set
{
  /* LR(0) items.  */
  ulib_vector items;
};
typedef struct xg_lr0set xg_lr0set;

/* Create an LR(0) set.  */
xg_lr0set *xg_lr0set_new ();

/* Add the item <PROD,DOT> to the set, if not already present.  Return
   negative on error, positive if the set changed (item not present)
   or zero otherwise.  */
int xg_lr0set_add (xg_lr0set *set, unsigned int prod, unsigned int dot);

/* Return the number of items in the set.  */
unsigned int xg_lr0set_count (const xg_lr0set *set);

/* Return the Nth item in the set.  */
const xg_lr0item *xg_lr0set_get_item (const xg_lr0set *set, unsigned int n);

/* Compute the closure of an LR(0) set.  */
struct xg_grammar;
int xg_lr0set_closure (const struct xg_grammar *g, xg_lr0set *set);

/* Display a debugging dump of an LR(0) set.  */
void xg_lr0set_debug (FILE *out, const struct xg_grammar *g,
                      const xg_lr0set *set);


/* Initialize LR(0) sets memory management.  */
int xg__init_lr0sets ();

END_DECLS

#endif /*  xg__lr0_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: d814b602-03a6-4916-a5a1-c6393afedf77
 * End:
 */
