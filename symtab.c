/* symtab.c - grammar definition parser symbol table implementation
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
#include "symtab.h"
#include <string.h>

/* Symbol table directory size.  */
#define SYMTAB_SIZE 211

/* Symbol definition hash function.  */
static unsigned int
symtab_hash (const ulib_list *lst)
{
  const xg_symbol_def *def = (const xg_symbol_def *) lst;

  return ulib_strhash (def->name);
}

/* Symbol definition compare function.  */
static int
symtab_cmp (const ulib_list *a, const ulib_list *b)
{
  const xg_symbol_def *def_a = (const xg_symbol_def *) a;
  const xg_symbol_def *def_b = (const xg_symbol_def *) b;

  return strcmp (def_a->name, def_b->name);
}

/* Initialize a symbol table.  */
int
xg_symtab_init (xg_symtab *tab)
{
  return ulib_hash_init (tab, SYMTAB_SIZE, symtab_hash, symtab_cmp);
}

/* Destroy a symbol table. */
void
xg_symtab_destroy (xg_symtab *tab)
{
  ulib_hash_destroy (tab);
}

/* Insert a symbol to the symbol table.  */
void
xg_symtab_insert (xg_symtab *tab, xg_symbol_def *def)
{
  ulib_hash_insert (tab, &def->list);
}

/* Find a symbol in the symbol table.  */
xg_symbol_def *
xg_symtab_lookup (const xg_symtab *tab, const char *name)
{
  xg_symbol_def *def;
  def->name = (char *) name;

  return (xg_symbol_def *) ulib_hash_lookup (tab, &def->list);
}
END_DECLS

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: d3877c6a-c044-4d99-9497-f56cbc76b5fa
 * End:
 */
