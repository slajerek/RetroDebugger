/** \file   debug_gtk3.c
 * \brief   Gtk3 port debugging code
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#include <string.h>
#include "archdep.h"

#include "debug_gtk3.h"


/** \brief  basename() implementation for debug messages
 *
 * A basename() implementation that doesn't modify its argument and doesn't
 * allocate anything.
 *
 * This way we can safely use this function in the debug_gtk3() macro. Other
 * versions of basename() either modify their argument or allocate memory for
 * a copy of the basename component or depend on a specific libc:
 *
 *  - POSIX.1-2001 and POSIX.1-2008 may modify their argument
 *  - GNU's version in <string.h> works like this function but depends on GNU's
 *    glibc
 *  - GLib had g_basename() which works like this function but was deprecated
 *    in favour of g_basename_get_path() which allocates memory
 *  - VICE's util_fname_split() allocates memory
 *
 * \param[in]   path    path
 *
 * \return  pointer to the basename in \a path or when no basename component is
 *          present a pointer to the terminating nul character in \a path
 *
 * \note    If \a path is `NULL`, `NULL` will be returned.
 */
const char *debug_gtk3_basename(const char *path)
{
    if (path != NULL && *path != '\0') {
        const char *p = path + strlen(path) - 1;

        /* scan for directory separator */
        while (p >= path && *p != ARCHDEP_DIR_SEP_CHR) {
            p--;
        }
        if (p >= path) {
            /* must be separator */
            path = p + 1;
        } /* else we 'ran off' the path, so no dirname component, return path */
    }
    return path;
}
