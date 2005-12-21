/* grammar.c - grammar structures utility functions
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
#include "grammar.h"
#include "xg.h"
#include <ulib/cache.h>
#include <ulib/bitset.h>

/* Symbol definition cache.  */
static ulib_cache *symbol_def_cache;

/* Symbol definition constructor.  */
static int
symbol_def_ctor (xg_symbol_def *def, unsigned int sz __attribute__ ((unused)))
{
  if (ulib_bitset_init (&def->first) == 0)
    {
      if (ulib_bitset_init (&def->follow) == 0)
        {
          if (ulib_vector_init (&def->prods, ULIB_ELT_SIZE,
                                sizeof (unsigned int), 0) == 0)
            return 0;
          ulib_bitset_destroy (&def->follow);
        }
      ulib_bitset_destroy (&def->first);
    }
  return -1;
}

/* Symbol definition destruction.  */
static void
symbol_def_clear (xg_symbol_def *def, unsigned int sz __attribute__ ((unused)))
{
  free (def->name);
  ulib_bitset_clear_all (&def->first);
  ulib_bitset_clear_all (&def->follow);
}

static void
symbol_def_dtor (xg_symbol_def *def, unsigned int sz __attribute__ ((unused)))
{
  ulib_bitset_destroy (&def->first);
  ulib_bitset_destroy (&def->follow);
}


/* Create a symbol definition.  */
xg_symbol_def *
xg_symbol_new (char *name)
{
  xg_symbol_def *def;

  if ((def = ulib_cache_alloc (symbol_def_cache)) != 0)
    {
      def->code = 0;
      def->name = name;
      def->terminal = 1;

      return def;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to allocate a symbol");
  return 0;
}

/* Add production N with DEF as its left hand side.  */
int
xg_symbol_def_add_production (xg_symbol_def *def, unsigned int n)
{
  return ulib_vector_append (&def->prods, &n);
}

/* Get the number of productions with DEF as their left hand side.  */
unsigned int
xg_symbol_def_production_count (const xg_symbol_def *def)
{
  return ulib_vector_length (&def->prods);
}

/* Get the Nth production number.  */
unsigned int
xg_symbol_def_get_production (const xg_symbol_def *def, unsigned int n)
{
  return *(unsigned int *) ulib_vector_elt (&def->prods, n);
}


/* Productions cache.  */
static ulib_cache *production_cache;

/* Create a production.  */
xg_production *
xg_production_new (xg_symbol lhs)
{
  xg_production *prod;

  prod = ulib_cache_alloc (production_cache);
  if (prod != 0)
    {
      prod->lhs = lhs;
      if (ulib_vector_init (&prod->rhs, ULIB_ELT_SIZE, sizeof (xg_symbol),
                            ULIB_GROWTH_SCALE, 2, 0) == 0)
        return prod;

      ulib_log_printf (xg_log, "ERROR: Unable to create production rhs");
      return 0;
    }

  ulib_log_printf (xg_log, "ERROR: Unable to allocate a production");
  return 0;
}

/* Production constructor.  */
static int
production_ctor (xg_production *prod, unsigned int sz __attribute__ ((unused)))
{
  if (ulib_vector_init (&prod->rhs, ULIB_ELT_SIZE, sizeof (xg_symbol),
                        ULIB_GROWTH_SCALE, 2, 0) == 0)
    return 0;

  ulib_log_printf (xg_log, "ERROR: Unable to create production rhs");
  return -1;
}

/* Production destruction.  */
static void
production_clear (xg_production *prod, unsigned int sz __attribute ((unused)))
{
  ulib_vector_set_size (&prod->rhs, 0);
}

static void
production_dtor (xg_production *prod, unsigned int sz __attribute__ ((unused)))
{
  ulib_vector_destroy (&prod->rhs);
}

/* Append a symbol to the right hand side.  */
int
xg_production_add (xg_production *prod, xg_symbol sym)
{
  int sts;

  if ((sts = ulib_vector_append (&prod->rhs, &sym)) != 0)
    ulib_log_printf (xg_log, "ERROR: Unable to extend production rhs");
  return sts;
}

/* Get the number of the symbols at the right hand side of a
   production.  */
unsigned int
xg_production_length (const xg_production *p)
{
  return ulib_vector_length (&p->rhs);
}

/* Get the Nth symbol from the right hand side of a production.  */
xg_symbol
xg_production_get_symbol (const xg_production *p, unsigned int n)
{
  return *(xg_symbol *) ulib_vector_elt (&p->rhs, n);
}


/* Grammar pointer scan function.  */
static int
grammar_gcscan (xg_grammar *g, void **ptr, unsigned int sz)
{
  int cnt = 0;
  unsigned int i, n;

  cnt = ulib_vector_length (&g->syms) -  XG_TOKEN_LITERAL_MAX;
  cnt += ulib_vector_length (&g->prods);

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

  if ((g = xg_malloc (sizeof (xg_grammar))) != 0)
    {
      g->start = 0;
      if (ulib_vector_init (&g->syms, ULIB_DATA_PTR_VECTOR, 0) == 0)
        {
          if (ulib_vector_resize (&g->syms, XG_TOKEN_LITERAL_MAX + 1) == 0)
            {
              if (ulib_vector_init (&g->prods, ULIB_DATA_PTR_VECTOR, 0) == 0)
                {
                  if (ulib_gcroot (g, (ulib_gcscan_func) grammar_gcscan) == 0)
                    return g;

                  ulib_vector_destroy (&g->prods);
                }
            }
          ulib_vector_destroy (&g->syms);
        }
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
xg_symbol
xg_grammar_add_symbol (xg_grammar *g, xg_symbol_def *def)
{
  if (ulib_vector_append_ptr (&g->syms, def) < 0)
    {
      ulib_log_printf (xg_log, "ERROR: Unable to add a symbol");
      return -1;
    }

  return ulib_vector_length (&g->syms) - 1;
}

/* Get the symbol definition for the symbol CODE.  */
xg_symbol_def *
xg_grammar_get_symbol (const xg_grammar *g, xg_symbol code)
{
  return (xg_symbol_def *) ulib_vector_ptr_elt (&g->syms, code);
}

/* Add a production to the grammar.  */
int
xg_grammar_add_production (xg_grammar *g, xg_production *p)
{
  return ulib_vector_append_ptr (&g->prods, p);
}

/* Get production count.  */
unsigned int
xg_grammar_production_count (const xg_grammar *g)
{
  return ulib_vector_length (&g->prods);
}

/* Get Nth production.  */
xg_production *
xg_grammar_get_production (const xg_grammar *g, unsigned int n)
{
  return (xg_production *) ulib_vector_ptr_elt (&g->prods, n);
}

/* Return true if the symbol SYM is a terminal.  */
int
xg_grammar_is_terminal_sym (const xg_grammar *g, xg_symbol sym)
{
  xg_symbol_def *def;

  if (sym < XG_TOKEN_LITERAL_MAX)
    return 1;

  def = xg_grammar_get_symbol (g, sym);

  return def == 0 ? 1 : def->terminal;
}


static int
init_caches (void)
{
  symbol_def_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_symbol_def),
                                        ULIB_CACHE_ALIGN, sizeof (unsigned int),
                                        ULIB_CACHE_CTOR, symbol_def_ctor,
                                        ULIB_CACHE_CLEAR, symbol_def_clear,
                                        ULIB_CACHE_DTOR, symbol_def_dtor,
                                        ULIB_CACHE_GC, 0);
  if (symbol_def_cache != 0)
    {
      production_cache =
        ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_production),
                           ULIB_CACHE_ALIGN, sizeof (void *),
                           ULIB_CACHE_CTOR, production_ctor,
                           ULIB_CACHE_CLEAR, production_clear,
                           ULIB_CACHE_DTOR, production_dtor,
                           ULIB_CACHE_GC, 0);
      if (production_cache != 0)
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
      if (ulib_bitset_init (eps_set) == 0)
        {
          if (ulib_bitset_set (eps_set, XG_EPSILON) == 0)
            {
              if ((eof_set = malloc (sizeof (ulib_bitset))) != 0)
                {
                  if (ulib_bitset_init (eof_set) == 0)
                    {
                      if (ulib_bitset_set (eof_set, XG_EOF) == 0)
                        {
                          xg_epsilon_set = eps_set;
                          xg_eof_set = eof_set;
                          return 0;
                        }
                      ulib_bitset_destroy (eof_set);
                    }
                  free (eof_set);
                }
            }
          ulib_bitset_destroy (eps_set);
        }
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
xg_symbol_name_debug (FILE *out, const xg_grammar *g, xg_symbol sym)
{
  xg_symbol_def *def;

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
xg_symbol_def_debug (FILE *out, const xg_grammar *g, const xg_symbol_def *def)
{
  fprintf (out, "Symbol %u (%s):\n\tname: %s\n", def->code,
           def->terminal ? "terminal" : "non-terminal", def->name);

  if (def->terminal == 0)
    {
      if (xg_symbol_def_production_count (def) != 0)
        {
          unsigned int i, n;

          fprintf (out, "\tProductions:");

          n = xg_symbol_def_production_count (def);
          for (i = 0; i < n; ++i)
            fprintf (out, " %u", xg_symbol_def_get_production (def, i));
          fputc ('\n', out);
        }

      fprintf (out, "\tFIRST: ");
      xg_symset_debug (out, g, &def->first);

      fprintf (out, "\tFOLLOW: ");
      xg_symset_debug (out, g, &def->follow);
    }
}

/* Display a debugging dump a production.  */
void
xg_production_debug (FILE *out, const xg_grammar *g, const xg_production *p)
{
  xg_symbol sym;
  xg_symbol_def *def;
  unsigned int i, n;

  def = xg_grammar_get_symbol (g, p->lhs);
  fprintf (out, "%s ->", def->name);

  n = xg_production_length (p);
  for (i = 0; i < n; ++i)
    {
      sym = xg_production_get_symbol (p, i);
      fputc (' ', out);
      xg_symbol_name_debug (out, g, sym);
    }
  fputc ('\n', out);
}

/* Display a debugging dump of the grammar.  */
void
xg_grammar_debug (FILE *out, const xg_grammar *g)
{
  unsigned int i, n;
  xg_production *p;
  xg_symbol_def *def;

  fputs ("Productions:\n------------\n\n", out);
  n = xg_grammar_production_count (g);
  for (i = 0; i < n; ++i)
    {
      p = xg_grammar_get_production (g, i);
      fprintf (out, "%4d: ", i);
      xg_production_debug (out, g, p);
    }

  fputs ("Symbols:\n--------\n\n", out);
  n = ulib_vector_length (&g->syms);
  for (i = 0; i < n; ++i)
    {
      def = xg_grammar_get_symbol (g, i);
      if (def != 0)
        xg_symbol_def_debug (out, g, def);
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: e85aaa11-d93a-414f-8851-eaca42e8404e
 * End:
 */
