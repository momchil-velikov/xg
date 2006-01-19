/* xg.c - main program
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
#include "lr0.h"
#include "lalr.h"
#include "gen-parser.h"
#include "xg.h"

#include <ulib/cache.h>
#include <stdio.h>

/* XG message log.  */
ulib_log *xg_log;

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
  if (xg__init_grammar () < 0 || xg__init_lr0dfa () < 0 || xg__init_lalr () < 0)
    goto error;

  /* Parse the input file. */
  if ((g = xg_grammar_read (argv [1])) == 0)
    goto error;

  /* Compute FIRST and FOLLOW sets.  */
  if (xg_grammar_compute_first (g) < 0 || xg_grammar_compute_follow (g) < 0)
    goto error;

  if (1)
    {
      xg_lr0dfa *dfa = xg_lr0dfa_new (g);
      xg_make_lalr_reductions (g, dfa);
      xg_resolve_conflicts (g, dfa);
      xg_grammar_debug (stderr, g);
      xg_lr0dfa_debug (stderr, g, dfa);
      xg_gen_c_parser (stdout, g, dfa);
      xg_lr0dfa_del (dfa);
    }
  else if (0)
    {
      xg_make_random_sentence (stdout, g, 10);
    }
  else
    {
      xg_grammar_debug (stderr, g);
    }
  xg_grammar_del (g);
  ulib_gcrun ();

  ulib_log_write (xg_log, stderr);
  return 0;

error:
  ulib_log_write (xg_log, stderr);
  return -1;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 1c5b50b4-ff60-4a95-a943-365b67baf14a
 * End:
 */
