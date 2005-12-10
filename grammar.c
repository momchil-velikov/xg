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

/* Symbol definition cache.  */
static ulib_cache *symbol_def_cache;

/* Create a symbol definition.  */
xg_symbol_def *
xg_symbol_new (char *name)
{
  xg_symbol_def *def;

  def = ulib_cache_alloc (symbol_def_cache);
  if (def == 0)
    ulib_log_printf (xg_log, "ERROR: Unable to allocate a symbol");
  else
    {
      def->code = 0;
      def->name = name;
      def->terminal = 0;
    }
  return def;
}

/* Symbol definition destructor.  */
static void
symbol_def_clear (xg_symbol_def *def, unsigned int sz __attribute__ ((unused)))
{
  free (def->name);
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
int
xg_production_length (xg_production *p)
{
  return ulib_vector_length (&p->rhs);
}

/* Get the Nth symbol from the right hand side of a production.  */
xg_symbol
xg_production_get_symbol (xg_production *p, unsigned int n)
{
  return *(xg_symbol *) ulib_vector_elt (&p->rhs, n);
}


/* Grammar pointer scan function.  */
static int
grammar_gcscan (xg_grammar *g, void **ptr, unsigned int sz)
{
  int cnt = 0;
  unsigned int i, n;

  cnt = ulib_vector_length (&g->syms) -  XG_TOKEN_LITERAL_MAX + g->ntoks;
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
      g->ntoks = 0;
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

/* Add a symbol definition to a grammat.  Return symbol index or
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
xg_grammar_get_symbol (xg_grammar *g, xg_symbol code)
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
int
xg_grammar_production_count (xg_grammar *g)
{
  return ulib_vector_length (&g->prods);
}

/* Get Nth production.  */
xg_production *
xg_grammar_get_production (xg_grammar *g, unsigned int n)
{
  return (xg_production *) ulib_vector_ptr_elt (&g->prods, n);
}


int
xg_init_grammar_caches (void)
{
  symbol_def_cache = ulib_cache_create (ULIB_CACHE_SIZE, sizeof (xg_symbol_def),
                                        ULIB_CACHE_ALIGN, sizeof (unsigned int),
                                        ULIB_CACHE_CLEAR, symbol_def_clear,
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

/* Display a debugging dump of the grammar.  */
void
xg_grammar_debug (FILE *out, xg_grammar *g)
{
  unsigned int i, j, np, ns;
  xg_production *p;
  xg_symbol sym;
  xg_symbol_def *def;

  np = xg_grammar_production_count (g);
  for (i = 1; i < np; ++i)
    {
      p = xg_grammar_get_production (g, i);
      def = xg_grammar_get_symbol (g, p->lhs);
      fprintf (out, "%4d: %s ->", i, def->name);

      ns = xg_production_length (p);
      for (j = 0; j < ns; ++j)
        {
          sym = xg_production_get_symbol (p, j);
          if (sym <= XG_TOKEN_LITERAL_MAX)
            fprintf (out, " '%c'", sym);
          else
            {
              def = xg_grammar_get_symbol (g, sym);
              fprintf (out, " %s", def->name);
            }
        }
      fputc ('\n', out);
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: e85aaa11-d93a-414f-8851-eaca42e8404e
 * End:
 */
