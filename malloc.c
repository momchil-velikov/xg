/* malloc.c - logging malloc wrappers.
 *
 * Copyright (C) 2006 Momchil Velikov
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
#include "xg.h"

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

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 5e130ebe-7346-4caf-9805-e331f49d2d9a
 * End:
 */
