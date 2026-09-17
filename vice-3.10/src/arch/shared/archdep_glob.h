/** \file   archdep_glob.h
 * \brief   very minimalistic glob()
 * \author  pottendo
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - BSD
 *  - MacOS
 *  - Haiku
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_ARCHDEP_GLOB_H
#define VICE_ARCHDEP_GLOB_H

#if defined(UNIX_COMPILE)
#if defined (HAVE_GLOB_H)
#include <glob.h>
#define archdep_glob glob
#define archdep_globfree globfree
#endif
#endif

#if defined (WINDOWS_COMPILE)
#include <stdlib.h>

typedef struct {
  size_t   gl_pathc;    /* Count of paths matched so far  */
  char   **gl_pathv;    /* List of matched pathnames.  */
  size_t   gl_offs;     /* Slots to reserve in gl_pathv.  */
} glob_t;

/* expects an absolute path for pattern and many glob() functions are NOT implemented */
int archdep_glob(const char *pattern, int flags, void *errfunc, glob_t *pglob);
void archdep_globfree(glob_t *pglob);

#endif

#endif
