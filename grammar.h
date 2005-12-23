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
#include <ulib/bitset.h>

#include <stdio.h>

BEGIN_DECLS

/* Symbol code.  */
typedef int xg_symbol;

/* Max literal token code.  */
#define XG_TOKEN_LITERAL_MAX 255

/* End of input marker code.  */
#define XG_EOF 0

/* Epsilon (empty sequence) code.  */
#define XG_EPSILON 1

/* A terminal set, containing only the empty symbol.  */
extern const ulib_bitset *xg_epsilon_set;

/* A terminal set, containing only the end of input symbol.  */
extern const ulib_bitset *xg_eof_set;

/* Grammar symbol definition.  */
struct xg_symbol_def
{
  /* Linked list for the symbol table.  */
  ulib_list list;

  /* Symbol code.  */
  xg_symbol code;

  /* Symbol name.  */
  char *name;

  /* FIRST set (for non-terminal symbols).  */
  ulib_bitset first;

  /* FOLLOW set (for non-terminal symbols).  */
  ulib_bitset follow;

  /* All productions, having this symbol as their left hand side.  */
  ulib_vector prods;

  /* Terminal flag.  */
  unsigned int terminal : 1;
};
typedef struct xg_symbol_def xg_symbol_def;

/* Create a symbol definition.  */
xg_symbol_def *xg_symbol_new (char *name);

/* Add production N with DEF as its left hand side.  */
int xg_symbol_def_add_production (xg_symbol_def *def, unsigned int n);

/* Get the number of productions with DEF as their left hand side.  */
unsigned int xg_symbol_def_production_count (const xg_symbol_def *def);

/* Get the Nth production number.  */
unsigned int xg_symbol_def_get_production (const xg_symbol_def *def,
                                           unsigned int n);

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
xg_production *xg_production_new (xg_symbol);

/* Delete a production.  */
void xg_production_del (xg_production *);

/* Append a symbol to the right hand side.  */
int xg_production_add (xg_production *, xg_symbol);

/* Get the number of the symbols at the right hand side of a
   production.  */
unsigned int xg_production_length (const xg_production *);

/* Get the Nth symbol from the right hand side of a production.  */
xg_symbol xg_production_get_symbol (const xg_production *, unsigned int n);


/* Grammar.  */
struct xg_grammar
{
  /* Start symbol code.  */
  xg_symbol start;

  /* All symbol definitions.  */
  ulib_vector syms;

  /* All productions.  */
  ulib_vector prods;
};
typedef struct xg_grammar xg_grammar;

/* Create an empty grammar structure.  */
xg_grammar *xg_grammar_new ();

/* Create a grammar by parsing a grammar description file.  */
xg_grammar *xg_grammar_read (const char *);

/* Delete a grammar structure.  */
void xg_grammar_del (xg_grammar *);

/* Add a symbol definition to a grammar.  Return symbol index or
   negative on error.  */
xg_symbol xg_grammar_add_symbol (xg_grammar *, xg_symbol_def *);

/* Get the symbol definition for the symbol CODE.  */
xg_symbol_def *xg_grammar_get_symbol (const xg_grammar *, xg_symbol code);

/* Add a production to the grammar.  */
int xg_grammar_add_production (xg_grammar *, xg_production *);

/* Get production count.  */
unsigned int xg_grammar_production_count (const xg_grammar *);

/* Get Nth production.  */
xg_production *xg_grammar_get_production (const xg_grammar *, unsigned int n);

/* Return true if the symbol SYM is a terminal.  */
int xg_grammar_is_terminal_sym (const xg_grammar *g, xg_symbol sym);

/* Compute the FIRST set for each non-terminal.  */
int xg_grammar_compute_first (const xg_grammar *g);

/* Compute the FOLLOW set for each non-terminal.  */
int xg_grammar_compute_follow (const xg_grammar *g);


/* Display a symbol name.  */
void xg_symbol_name_debug (FILE *out, const xg_grammar *g, xg_symbol sym);

/* Display a set of symbols.  */
void xg_symset_debug (FILE *out, const xg_grammar *g, const ulib_bitset *set);

/* Display a debugging dump of a symbol.  */
void xg_symbol_def_debug (FILE *out, const xg_grammar *g,
                          const xg_symbol_def *def);

/* Display a debugging dump a production.  */
void xg_production_debug (FILE *out, const xg_grammar *g,
                          const xg_production *p);

/* Display a debugging dump of the grammar.  */
void xg_grammar_debug (FILE *, const xg_grammar *);


/* Initialize grammars memory management.  */
int xg__init_grammar (void);

END_DECLS
#endif /* xg__grammar_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 2ad60633-99ba-4b78-b96d-d8b8a026dc12
 * End:
 */
