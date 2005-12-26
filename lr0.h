/* lr0.h - LR(0) DFA declarations
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

#include "grammar.h"
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

/* A parse action in an LR(0) DFA state.  */
struct xg_lr0axn
{
  /* Lookahead.  */
  xg_sym sym;

  /* Shift or reduce (non-terminal transitions are encodef as
     shifts).  */
  unsigned int shift;

  /* State or production number.  */
  unsigned int no;
};
typedef struct xg_lr0axn xg_lr0axn;

/* A state in the LR(0) DFA for viable prefixes.  */
struct xg_lr0state
{
  /* LR(0) items.  */
  ulib_vector items;

  /* Parse actions.  */
  ulib_vector axns;
};
typedef struct xg_lr0state xg_lr0state;

/* Create an LR(0) DFA state.  */
xg_lr0state *xg_lr0state_new ();

/* Add the item <PROD,DOT> to the state, if not already present.
   Return negative on error, positive if the state changed (item not
   present) or zero otherwise.  */
int xg_lr0state_add_item (xg_lr0state *set, unsigned int prod,
                          unsigned int dot);

/* Return the number of LR(0) items in the state.  */
unsigned int xg_lr0state_item_count (const xg_lr0state *state);

/* Return the N-th item in the state.  */
xg_lr0item *xg_lr0state_get_item (const xg_lr0state *state, unsigned int n);

/* Return a pointer to an array of LR(0) items.  The pointer is
   possibly invalidated after adding an item to the set.  */
xg_lr0item *xg_lr0state_items_front (const xg_lr0state *state);

/* Return a pointer just after last LR(0) item.  The pointer is
   possibly invalidated after adding an item to the set.  */
xg_lr0item *xg_lr0state_items_back (const xg_lr0state *state);

/* Add a parse action to an LR(0) state.  */
int xg_lr0state_add_axn (xg_lr0state *state, xg_sym sym, unsigned int shift,
                         unsigned int no);

/* Get parse actions count.  */
unsigned int xg_lr0state_axn_count (const xg_lr0state *state);

/* Get a parse action.  */
const xg_lr0axn *xg_lr0state_get_axn (const xg_lr0state *state, unsigned int n);

/* Compute the closure of an LR(0) state.  */
int xg_lr0state_closure (const xg_grammar *g, xg_lr0state *state);

/* Compute the goto (STATE, SYM) function.  */
xg_lr0state *xg_lr0state_goto (const xg_grammar *g, const xg_lr0state *src,
                               xg_sym sym);

/* Display a debugging dump of an LR(0) state.  */
void xg_lr0state_debug (FILE *out, const xg_grammar *g,
                        const xg_lr0state *state);


/* LR(0) DFA.  */
struct xg_lr0dfa
{
  /* Automaton states (pointers).  */
  ulib_vector states;
};
typedef struct xg_lr0dfa xg_lr0dfa;

/* Create an LR(0) DFA.  */
xg_lr0dfa *xg_lr0dfa_new (const xg_grammar *g);

/* Delete an LR(0) DFA.  */
void xg_lr0dfa_del (xg_lr0dfa *dfa);

/* Get the number of LR(0) DFA states.  */
unsigned int xg_lr0dfa_state_count (const xg_lr0dfa *dfa);

/* Get the N-th  LR(0) DFA state.  */
xg_lr0state *xg_lr0dfa_get_state (const xg_lr0dfa *dfa, unsigned int n);

/* Display a debugging dump of an LR(0) DFA.  */
void xg_lr0dfa_debug (FILE *out, const xg_grammar *g, const xg_lr0dfa *dfa);


/* Initialize LR(0) DFA memory management.  */
int xg__init_lr0dfa ();

END_DECLS

#endif /*  xg__lr0_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: d814b602-03a6-4916-a5a1-c6393afedf77
 * End:
 */
