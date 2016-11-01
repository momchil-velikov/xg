/* xg.h - miscelaneous definitions
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
 * along with XG; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
 */
#ifndef xg__xg_h
#define xg__xg_h 1

#include <ulib/log.h>
#include <stdlib.h>

BEGIN_DECLS

/* XG message log.  */
extern ulib_log *xg_log;

/* Logging memory allocation functions.  */
void *xg_malloc(size_t);
void *xg_calloc(size_t, size_t);
void *xg_realloc(void *, size_t);
static inline void
xg_free(void *ptr) {
    free(ptr);
}

END_DECLS

#endif /*  xg__xg_h */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * End:
 */
