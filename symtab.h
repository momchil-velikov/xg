/* symtab.h - grammar definition parser symbol table
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
#ifndef xg__symtab_h
#define xg__symtab_h 1

#include <ulib/defs.h>
#include <ulib/hash.h>

#include "grammar.h"

BEGIN_DECLS

/* Symbol table -- a hash table of symbol definitions.  */
typedef ulib_hash xg_symtab;

/* Initialize a symbol table.  */
int xg_symtab_init (xg_symtab *);

/* Destroy a symbol table. */
void xg_symtab_destroy (xg_symtab *);

/* Insert a symbol to the symbol table.  */
void xg_symtab_insert (xg_symtab *, xg_symdef *);

/* Find a symbol in the symbol table.  */
xg_symdef *xg_symtab_lookup (const xg_symtab *, const char *);

END_DECLS
#endif /* xg__symtab_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: b15ef3bd-9f31-492b-a4ac-c46837d47154
 * End:
 */
