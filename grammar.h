/* grammar.h - grammar data structures
 *
 * Copyright (C) 2005, 2006 Momchil Velikov
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
#ifndef xg__grammar_h
#define xg__grammar_h 1

#include <ulib/vector.h>
#include <ulib/list.h>
#include <ulib/bitset.h>

#include <stdio.h>

BEGIN_DECLS

/* Symbol code.  */
typedef int xg_sym;

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

/* Symbol associativity.  */
enum xg_assoc
{
  xg_assoc_unknown,
  xg_assoc_none,
  xg_assoc_left,
  xg_assoc_right
};

/* Symbol kind.  */
enum xg_sym_kind
{
  xg_implicit_terminal,
  xg_explicit_terminal,
  xg_non_terminal
};

/* Grammar symbol definition.  */
struct xg_symdef
{
  /* Linked list for the symbol table.  */
  ulib_list list;

  /* Symbol code.  */
  xg_sym code;

  /* Symbol name.  */
  char *name;

  /* FIRST set (for non-terminal symbols).  */
  ulib_bitset first;

  /* FOLLOW set (for non-terminal symbols).  */
  ulib_bitset follow;

  /* All productions, having this symbol as their left hand side.  */
  ulib_vector prods;

  /* Terminal flag.  */
  unsigned int terminal : 2;

  /* Precedence.  */
  unsigned int prec : 16;

  /* Associtivity.  */
  unsigned int assoc : 2;
};
typedef struct xg_symdef xg_symdef;

/* Create a symbol definition (consume the argument).  */
xg_symdef *xg_symdef_new (char *name);

/* Create a symbol definition (copy the argument).  */
xg_symdef *xg_symdef_new_copy (const char *name);

/* Add production N with DEF as its left hand side.  */
int xg_symdef_add_prod (xg_symdef *def, unsigned int n);

/* Get the number of productions with DEF as their left hand side.  */
unsigned int xg_symdef_prod_count (const xg_symdef *def);

/* Get the Nth production number.  */
unsigned int xg_symdef_get_prod (const xg_symdef *def, unsigned int n);

/* Sentenial form: vector of symbol codes.  */
typedef ulib_vector xg_sentenial_form;

/* Grammar production.  */
struct xg_prod
{
  /* Left hand side: non-terminal symbol code.  */
  xg_sym lhs;

  /* Right hand side: sentenial form.  */
  xg_sentenial_form rhs;

  /* Rightmost terminal.  */
  xg_sym prec;
};
typedef struct xg_prod xg_prod;

/* Create a production.  */
xg_prod *xg_prod_new (xg_sym lhs);

/* Append a symbol to the right hand side.  */
int xg_prod_add (xg_prod *prod, xg_sym sym);

/* Get the number of the symbols at the right hand side of a
   production.  */
unsigned int xg_prod_length (const xg_prod *);

/* Get the Nth symbol from the right hand side of a production.  */
xg_sym xg_prod_get_symbol (const xg_prod *, unsigned int n);

/* Get the right hand side of a production.  */
xg_sym *xg_prod_get_symbols (const xg_prod *);


/* Grammar.  */
struct xg_grammar
{
  /* Start symbol code.  */
  xg_sym start;

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
xg_sym xg_grammar_add_symbol (xg_grammar *, xg_symdef *);

/* Set the definition of a symbol with code SYM.  */
int xg_grammar_set_symbol (xg_grammar *, xg_sym, xg_symdef *);

/* Get the symbol definition for the symbol CODE.  */
xg_symdef *xg_grammar_get_symbol (const xg_grammar *, xg_sym code);

/* Get symbol count.  The returned value is always bigger than
   XG_TOKEN_LITERAL_MAX.  */
int xg_grammar_symbol_count (const xg_grammar *g);

/* Add a production to the grammar.  */
int xg_grammar_add_prod (xg_grammar *, xg_prod *);

/* Get production count.  */
unsigned int xg_grammar_prod_count (const xg_grammar *);

/* Get Nth production.  */
xg_prod *xg_grammar_get_prod (const xg_grammar *, unsigned int n);

/* Print a production.  */
void xg_prod_print (FILE *out, const xg_grammar *g, const xg_prod *p);

/* Return true if the symbol SYM is a terminal.  */
int xg_grammar_is_terminal_sym (const xg_grammar *g, xg_sym sym);

/* Compute the FIRST set for each non-terminal.  */
int xg_grammar_compute_first (const xg_grammar *g);

/* Compute the FOLLOW set for each non-terminal.  */
int xg_grammar_compute_follow (const xg_grammar *g);


/* Check whether the symbol S is nullable.  */
int xg_nullable_sym (const xg_grammar *g, xg_sym s);

/* Check whether the sentenial form FORM can derive the empty string.
   The FIRST set is a prerequisite for calling this function.  */
int xg_nullable_form (const xg_grammar *g, unsigned int n, const xg_sym *form);


/* Output to OUT a random sentence from the language, defined by the
   grammar G.  The parameter SIZE indirectly influences the length of
   the generated sentence.  */
int xg_make_random_sentence (FILE *out, const xg_grammar *g, unsigned int size,
                             int names);


/* Display a symbol name.  */
void xg_symbol_name_debug (FILE *out, const xg_grammar *g, xg_sym sym);

/* Display a set of symbols.  */
void xg_symset_debug (FILE *out, const xg_grammar *g, const ulib_bitset *set);

/* Display a debugging dump of a symbol.  */
void xg_symdef_debug (FILE *out, const xg_grammar *g, const xg_symdef *def);

/* Display a debugging dump a production.  */
void xg_prod_debug (FILE *out, const xg_grammar *g, const xg_prod *p);

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
 * End:
 */
