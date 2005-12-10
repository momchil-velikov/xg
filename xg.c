/* xg.c - main program
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
#include <stdio.h>
#include "grammar.h"
#include "xg.h"
#include <ulib/cache.h>

/* XG message log.  */
ulib_log *xg_log;

/* Memory functions.  */
void *
xg_malloc (size_t sz)
{
  void *ptr;

  ptr = malloc (sz);
  if (ptr == 0 && sz != 0)
    ulib_log_printf (xg_log, "ERROR: Out of memory allocating %lu bytes",
                     (unsigned long) sz);
  return ptr;
}

void *
xg_calloc (size_t n, size_t sz)
{
  void *ptr;
  
  ptr = calloc (n, sz);
  if (ptr == 0 && n * sz != 0)
    ulib_log_printf (xg_log, "ERROR: Out of memory allocating %lu bytes",
                     (unsigned long) n * sz);
  return ptr;
}

void *
xg_realloc (void *oldptr, size_t sz)
{
  void *ptr;

  ptr = realloc (oldptr, sz);
  if (ptr == 0 && sz != 0)
    ulib_log_printf (xg_log, "ERROR: Out of memory allocating %lu bytes",
                     (unsigned long) sz);
  return ptr;
}

static int
usage ()
{
  fprintf (stderr, "usage: xg <filename>\n");
  return -1;
}

int
main (int argc, char *argv [])
{
  static ulib_log log;
  xg_grammar *g;

  if (argc != 2)
    return usage ();

  /* Create the message log.  */
  xg_log = &log;
  if (ulib_log_init (xg_log, "xg") < 0)
    {
      fprintf (stderr, "xg: ERROR: unable to create the message log.\n");
      return -1;
    }

  /* Initialize memory management.  */
  if (xg_init_grammar_caches () < 0)
    {
      ulib_log_write (xg_log, stderr);
      return -1;
    }

  g = xg_grammar_read (argv [1]);
  if (g == 0)
    {
      ulib_log_write (xg_log, stderr);
      return -1;
    }
  else
    {
      xg_grammar_debug (stdout, g);
      xg_grammar_del (g);
      ulib_gcrun ();
      return 0;
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 1c5b50b4-ff60-4a95-a943-365b67baf14a
 * End:
 */