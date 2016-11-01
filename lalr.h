/* lalr.h - LALR(1) look-ahead computation related declarations.  
 *
 * Copyright (C) 2006 Momchil Velikov
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

#ifndef xg__lalr_h
#define xg__lalr_h 1

#include "lr0.h"
#include <ulib/vector.h>

BEGIN_DECLS

/* LALR(1) transition.  */
struct xg_lalr_trans
{
  /* Related LALR transitions.  */
  ulib_vector rel;

  /* Function value.  */
  ulib_bitset value;
};
typedef struct xg_lalr_trans xg_lalr_trans;

/* Create reductions for an LALR(1) parser.  */
int xg_make_lalr_reductions (const xg_grammar *g, xg_lr0dfa *dfa);

/* Initialized the LALR(1) memory management.  */
int xg__init_lalr (void);

END_DECLS

#endif /* xg__lalr_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 70636750-d60c-48bc-9f5f-0e767411ecad
 * End:
 */
