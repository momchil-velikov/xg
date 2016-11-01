/* grammar.c - grammar structures utility functions
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
#include "grammar.h"
#include "xg.h"
#include <ulib/cache.h>
#include <ulib/bitset.h>
#include <assert.h>

/* Symbol definition cache.  */
static ulib_cache *symdef_cache;

/* Symbol definition constructor.  */
static int
symdef_ctor (xg_symdef *def, unsigned int sz __attribute__ ((unused)))
{
  (void) ulib_bitset_init (&def->first);
  (void) ulib_bitset_init (&def->follow);
  (void) ulib_vector_init (&def->prods, ULIB_ELT_SIZE, sizeof (unsigned), 0);
  return 0;
}

/* Symbol definition destruction.  */
static void
symdef_clear (xg_symdef *def, unsigned int sz __attribute__ ((unused)))
{
  free (def->name);
  ulib_bitset_clear_all (&def->first);
  ulib_bitset_clear_all (&def->follow);
  ulib_vector_set_size (&def->prods, 0);
}

static void
symdef_dtor (xg_symdef *def, unsigned int sz __attribute__ ((unused)))
{
  ulib_bitset_destroy (&def->first);
  ulib_bitset_destroy (&def->follow);
  ulib_vector_destroy (&def->prods);
}


/* Create a symbol definition (consume the argument).  */
xg_symdef *
xg_symdef_new (char *name)
{
  xg_symdef *def;

  if ((def = ulib_cache_alloc (symdef_cache)) != 0)
    {
      def->code = 0;
      def->name = name;
      def->terminal = xg_implicit_terminal;
      def->prec = 0;
      def->assoc = xg_assoc_unknown;

      return def;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to allocate a symbol");
  return 0;
}

/* Create a symbol definition (copy the argument).  */
xg_symdef *
xg_symdef_new_copy (const char *_name)
{
  char *name;
  unsigned int n;
  xg_symdef *def;

  n = strlen (_name);
  if ((name = xg_malloc (n + 1)) != 0)
    {
      memcpy (name, _name, n + 1);
      if ((def = xg_symdef_new (name)) != 0)
        return def;
      xg_free (name);
    }

  return 0;
}

/* Add production N with DEF as its left hand side.  */
int
xg_symdef_add_prod (xg_symdef *def, unsigned int n)
{
  return ulib_vector_append (&def->prods, &n);
}

/* Get the number of productions with DEF as their left hand side.  */
unsigned int
xg_symdef_prod_count (const xg_symdef *def)
{
  return ulib_vector_length (&def->prods);
}

/* Get the Nth production number.  */
unsigned int
xg_symdef_get_prod (const xg_symdef *def, unsigned int n)
{
  return *(unsigned int *) ulib_vector_elt (&def->prods, n);
}


/* Productions cache.  */
static ulib_cache *prod_cache;

/* Create a production.  */
xg_prod *
xg_prod_new (xg_sym lhs)
{
  xg_prod *prod;

  prod = ulib_cache_alloc (prod_cache);
  if (prod != 0)
    {
      prod->lhs = lhs;
      (void) ulib_vector_init (&prod->rhs, ULIB_ELT_SIZE, sizeof (xg_sym), 0);
      prod->prec = XG_EPSILON;
      return prod;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to allocate a production");
  return 0;
}

/* Production constructor.  */
static int
prod_ctor (xg_prod *prod, unsigned int sz __attribute__ ((unused)))
{
  (void) ulib_vector_init (&prod->rhs, ULIB_ELT_SIZE, sizeof (xg_sym), 0);
  return 0;
}

/* Production destruction.  */
static void
prod_clear (xg_prod *prod, unsigned int sz __attribute ((unused)))
{
  ulib_vector_set_size (&prod->rhs, 0);
}

static void
prod_dtor (xg_prod *prod, unsigned int sz __attribute__ ((unused)))
{
  ulib_vector_destroy (&prod->rhs);
}

/* Append a symbol to the right hand side.  */
int
xg_prod_add (xg_prod *prod, xg_sym sym)
{
  int sts;

  if ((sts = ulib_vector_append (&prod->rhs, &sym)) != 0)
    ulib_log_printf (xg_log, "ERROR: Unable to extend production rhs");
  return sts;
}

/* Get the number of the symbols at the right hand side of a
   production.  */
unsigned int
xg_prod_length (const xg_prod *p)
{
  return ulib_vector_length (&p->rhs);
}

/* Get the Nth symbol from the right hand side of a production.  */
xg_sym
xg_prod_get_symbol (const xg_prod *p, unsigned int n)
{
  return *(xg_sym *) ulib_vector_elt (&p->rhs, n);
}

/* Get the right hand side of a production.  */
xg_sym *
xg_prod_get_symbols (const xg_prod *p)
{
  return ulib_vector_front (&p->rhs);
}


/* Grammar pointer scan function.  */
static int
grammar_gcscan (xg_grammar *g, void **ptr, unsigned int sz)
{
  int cnt = 0;
  unsigned int i, n;

  cnt = ulib_vector_length (&g->syms) + ulib_vector_length (&g->prods);

  if ((unsigned int) cnt > sz)
    return -cnt;

  cnt = 0;
  n = ulib_vector_length (&g->syms);
  for (i = 0; i < n; ++i)
    {
      void *p = ulib_vector_ptr_elt (&g->syms, i);
      if (p != 0)
        ptr [cnt++] = p;
    }

  n = ulib_vector_length (&g->prods);
  for (i = 0; i < n; ++i)
    ptr [cnt++] = ulib_vector_ptr_elt (&g->prods, i);

  return cnt;
}

/* Create an empty grammar structure.  */
xg_grammar *
xg_grammar_new ()
{
  xg_grammar *g;
  xg_symdef *rsv, *err, *eof, *eps;

  if ((g = xg_malloc (sizeof (xg_grammar))) != 0)
    {
      g->start = 0;
      (void) ulib_vector_init (&g->syms, ULIB_DATA_PTR_VECTOR, 0);
      if (ulib_vector_resize (&g->syms, XG_TOKEN_LITERAL_MAX + 1) == 0)
        {
          if ((rsv = xg_symdef_new_copy ("<reserved>")) != 0
              && xg_grammar_add_symbol (g, rsv) == 0
              && (err = xg_symdef_new_copy ("<error>")) != 0
              && xg_grammar_add_symbol (g, err) == 0
              && (eof = xg_symdef_new_copy ("<eof>")) != 0
              && xg_grammar_set_symbol (g, XG_EOF, eof) == 0
              && (eps = xg_symdef_new_copy ("<eps>")) != 0
              && xg_grammar_set_symbol (g, XG_EPSILON, eps) == 0)
            {
              rsv->terminal = xg_explicit_terminal;
              err->terminal = xg_explicit_terminal;
              eof->terminal = xg_explicit_terminal;
              eps->terminal = xg_explicit_terminal;

              (void) ulib_vector_init (&g->prods, ULIB_DATA_PTR_VECTOR, 0);

              if (ulib_gcroot (g, (ulib_gcscan_func) grammar_gcscan) == 0)
                return g;
            }
          ulib_vector_destroy (&g->prods);
        }
      ulib_vector_destroy (&g->syms);
      xg_free (g);
    }

  ulib_log_printf (xg_log, "ERROR: Unable to allocate a grammar");
  return 0;
}

/* Delete a grammar structure.  */
void
xg_grammar_del (xg_grammar *g)
{
  ulib_vector_destroy (&g->syms);
  ulib_vector_destroy (&g->prods);
  ulib_gcunroot (g);
  xg_free (g);
}

/* Add a symbol definition to a grammar.  Return symbol index or
   negative on error.  */
xg_sym
xg_grammar_add_symbol (xg_grammar *g, xg_symdef *def)
{
  if (ulib_vector_append_ptr (&g->syms, def) < 0)
    {
      ulib_log_printf (xg_log, "ERROR: Unable to add a symbol");
      return -1;
    }

  def->code = ulib_vector_length (&g->syms) - 1;
  return 0;
}

/* Set the definition of a symbol with code SYM.  */
int
xg_grammar_set_symbol (xg_grammar *g, xg_sym sym, xg_symdef *def)
{
  assert (sym <= XG_TOKEN_LITERAL_MAX);
  assert (ulib_vector_ptr_elt (&g->syms, sym) == 0);

  def->code = sym;
  if (ulib_vector_set_ptr (&g->syms, sym, def) == 0)
    return 0;

  ulib_log_printf (xg_log, "ERROR: Unable to set a symbol definition");
  return -1;
}

/* Get the symbol definition for the symbol CODE.  */
xg_symdef *
xg_grammar_get_symbol (const xg_grammar *g, xg_sym code)
{
  return (xg_symdef *) ulib_vector_ptr_elt (&g->syms, code);
}

/* Get symbol count.  The returned value is always bigger than
   XG_TOKEN_LITERAL_MAX.  */
int
xg_grammar_symbol_count (const xg_grammar *g)
{
  return ulib_vector_length (&g->syms);
}

/* Add a production to the grammar.  */
int
xg_grammar_add_prod (xg_grammar *g, xg_prod *p)
{
  return ulib_vector_append_ptr (&g->prods, p);
}

/* Get production count.  */
unsigned int
xg_grammar_prod_count (const xg_grammar *g)
{
  return ulib_vector_length (&g->prods);
}

/* Get Nth production.  */
xg_prod *
xg_grammar_get_prod (const xg_grammar *g, unsigned int n)
{
  return (xg_prod *) ulib_vector_ptr_elt (&g->prods, n);
}

/* Return true if the symbol SYM is a terminal.  */
int
xg_grammar_is_terminal_sym (const xg_grammar *g, xg_sym sym)
{
  xg_symdef *def;

  if (sym < XG_TOKEN_LITERAL_MAX)
    return 1;

  def = xg_grammar_get_symbol (g, sym);

  return def->terminal != xg_non_terminal;
}


static int
init_caches (void)
{
  symdef_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_symdef),
                                    ULIB_CACHE_ALIGN, sizeof (unsigned int),
                                    ULIB_CACHE_CTOR, symdef_ctor,
                                    ULIB_CACHE_CLEAR, symdef_clear,
                                    ULIB_CACHE_DTOR, symdef_dtor,
                                    ULIB_CACHE_GC, 0);
  if (symdef_cache != 0)
    {
      prod_cache =
        ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_prod),
                           ULIB_CACHE_ALIGN, sizeof (void *),
                           ULIB_CACHE_CTOR, prod_ctor,
                           ULIB_CACHE_CLEAR, prod_clear,
                           ULIB_CACHE_DTOR, prod_dtor,
                           ULIB_CACHE_GC, 0);
      if (prod_cache != 0)
        return 0;
      else
        ulib_log_printf (xg_log,
                         "ERROR: Unable to create the production cache");
    }
  else
    ulib_log_printf (xg_log, "ERROR: Unable to create the symbols cache");

  return -1;
}

static int
init_bitsets (void)
{
  ulib_bitset *eps_set, *eof_set;

  if ((eps_set = malloc (sizeof (ulib_bitset))) != 0)
    {
      (void) ulib_bitset_init (eps_set);
      if (ulib_bitset_set (eps_set, XG_EPSILON) == 0)
        {
          if ((eof_set = malloc (sizeof (ulib_bitset))) != 0)
            {
              (void) ulib_bitset_init (eof_set);
              if (ulib_bitset_set (eof_set, XG_EOF) == 0)
                {
                  xg_epsilon_set = eps_set;
                  xg_eof_set = eof_set;
                  return 0;
                }
              ulib_bitset_destroy (eof_set);
              free (eof_set);
            }
        }
      ulib_bitset_destroy (eps_set);
      free (eps_set);
    }

  ulib_log_printf (xg_log, "ERROR: Unable to create the terminal sets");
  return -1;
}

int
xg__init_grammar (void)
{
  if (init_caches () < 0 || init_bitsets () < 0)
    return -1;
  else
    return 0;
}


void
xg_symbol_name_debug (FILE *out, const xg_grammar *g, xg_sym sym)
{
  xg_symdef *def;

  if (sym == XG_EOF)
    fputs ("<eof>", out);
  else if (sym == XG_EPSILON)
    fputs ("<eps>", out);
  else if (sym <= XG_TOKEN_LITERAL_MAX)
    fprintf (out, "'%c'", sym);
  else
    {
      def = xg_grammar_get_symbol (g, sym);
      fprintf (out, "%s", def->name);
    }
}

void
xg_symset_debug (FILE *out, const xg_grammar *g, const ulib_bitset *set)
{
  unsigned int i, n;

  n = ulib_bitset_max (set);
  for (i = 0; i < n; ++i)
    {
      if (ulib_bitset_is_set (set, i))
        {
          xg_symbol_name_debug (out, g, i);
          fputc (' ', out);
        }
    }
  fputc ('\n', out);
}

void
xg_symdef_debug (FILE *out, const xg_grammar *g, const xg_symdef *def)
{
  fprintf (out, "Symbol %u [%s",
           def->code,
           def->terminal == xg_non_terminal? "non-terminal" : "terminal");

  if (def->terminal != xg_non_terminal)
    fprintf (out, ", %s, %u]:\n",
             (def->assoc == xg_assoc_unknown
              ? "unknown"
              : def->assoc == xg_assoc_none
              ? "none"
              : def->assoc == xg_assoc_left
              ? "left" : "right"),
             def->prec);
  else
    fputs ("]:\n", out);

  if (def->name)
    fprintf (out, "\tname: %s\n", def->name);
  else
    fprintf (out, "\tname: '%c'\n", def->code);

  if (def->terminal == xg_non_terminal)
    {
      if (xg_symdef_prod_count (def) != 0)
        {
          unsigned int i, n;

          fprintf (out, "\tProductions:");

          n = xg_symdef_prod_count (def);
          for (i = 0; i < n; ++i)
            fprintf (out, " %u", xg_symdef_get_prod (def, i));
          fputc ('\n', out);
        }

      fprintf (out, "\tFIRST: ");
      xg_symset_debug (out, g, &def->first);

      fprintf (out, "\tFOLLOW: ");
      xg_symset_debug (out, g, &def->follow);
    }
}


/* Print a production.  */
void
xg_prod_print (FILE *out, const xg_grammar *g, const xg_prod *p)
{
  unsigned int i, n;
  xg_sym sym;
  const xg_symdef *def;

  def = xg_grammar_get_symbol (g, p->lhs);
  fprintf (out, "%s ->", def->name);

  n = xg_prod_length (p);
  for (i = 0; i < n; ++i)
    {
      sym = xg_prod_get_symbol (p, i);
      fputc (' ', out);
      xg_symbol_name_debug (out, g, sym);
    }
}

/* Display a debugging dump of a production.  */
void
xg_prod_debug (FILE *out, const xg_grammar *g, const xg_prod *p)
{
  xg_symdef *def;

  def = xg_grammar_get_symbol (g, p->prec);
  if (def != 0)
    {
      fprintf (out, " [%7s, %u] ",
               (def->assoc == xg_assoc_unknown
                ? "unknown"
                : def->assoc == xg_assoc_none
                ? "none"
                : def->assoc == xg_assoc_left
                ? "left" : "right"),
               def->prec);
    }
  else
    fputs (" [unknown, 0] ", out);

  xg_prod_print (out, g, p);
  fputc ('\n', out);
}

/* Display a debugging dump of the grammar.  */
void
xg_grammar_debug (FILE *out, const xg_grammar *g)
{
  unsigned int i, n;
  xg_prod *p;
  xg_symdef *def;

  fputs ("\nProductions:\n============\n\n", out);
  n = xg_grammar_prod_count (g);
  for (i = 0; i < n; ++i)
    {
      p = xg_grammar_get_prod (g, i);
      fprintf (out, "%4d: ", i);
      xg_prod_debug (out, g, p);
    }

  fputs ("\nSymbols:\n========\n\n", out);
  n = ulib_vector_length (&g->syms);
  for (i = 0; i < n; ++i)
    {
      def = xg_grammar_get_symbol (g, i);
      if (def != 0)
        xg_symdef_debug (out, g, def);
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * End:
 */
