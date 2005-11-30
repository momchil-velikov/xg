/* grammar.h - grammar data structures
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
#ifndef xg__grammar_h
#define xg__grammar_h 1

#include <ulib/defs.h>
#include <ulib/vector.h>
#include <ulib/list.h>

BEGIN_DECLS

/* Symbol code.  */
typedef unsigned int xg_symbol;

/* Grammar symbol definition.  */
struct xg_symbol_def
{
  /* Linked list for the symbol table.  */
  ulib_list list;

  /* Symbol code.  */
  xg_symbol code;

  /* Symbol name.  */
  char *name;

  /* Terminal flag.  */
  int terminal : 1;
};
typedef struct xg_symbol_def xg_symbol_def;

/* Create a symbol definition.  */
xg_symbol_def *xg_make_symbol (char *name);

/* Sentenial form: vector of symbol codes.  */
typedef ulib_vector xg_sentenial_form;

/* Grammar production.  */
struct xg_production
{
  /* Left hand side: non-terminal symbol code.  */
  xg_symbol lhs;

  /* Right hand side: sentenial form.  */
  xg_sentenial_form rhs;
};
typedef struct xg_production xg_production;

/* Create a production.  */
xg_production *xg_make_production (xg_symbol);

/* Append a symbol to the right hand side.  */
int xg_production_add (xg_production *, xg_symbol);

/* Grammar.  */
struct xg_grammar
{
  /* Start symbol code.  */
  xg_symbol start;

  /* All symbol definitions.  */
  ulib_vector syms;

  /* All productions.  */
  ulib_vector prod;
};
typedef struct xg_grammar xg_grammar;

/* Create a grammar by parsing a grammar description file.  */
xg_grammar *xg_grammar_read (const char *);

END_DECLS
#endif /* xg__grammar_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 2ad60633-99ba-4b78-b96d-d8b8a026dc12
 * End:
 */
