/* gen-c-slr.h - C SLR(1) declarations.  
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
#ifndef xg_gen_c_slr_h
#define xg_gen_c_slr_h 1

#include <ulib/defs.h>
#include "lr0.h"

BEGIN_DECLS

/* Generate an SLR(1) parser in ISO C.  */
int xg_gen_c_slr (FILE *out, const xg_grammar *g, const xg_lr0dfa *dfa);

END_DECLS
#endif /* xg_gen_c_slr_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 243a698d-a43d-4f7e-92b9-720ca0b15c8d
 * End:
 */
