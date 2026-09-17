/** \file   archdep_glob.c
 * \brief   very minimalistic glob()
 *
 * Wrapper for the POSIX glob(3) function.
 *
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

#include "vice.h"

#if !defined(UNIX_COMPILE) && !defined(HAIKU_COMPILE) && !defined(WINDOWS_COMPILE)
# error "Unsupported OS!"
#endif

#if defined(WINDOWS_COMPILE)
#include <archdep_glob.h>
#include <archdep_defs.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "lib.h"
#include "util.h"

static char *fh[1];

/* expects an absolute path for pattern and many glob() functions are NOT implemented */
int archdep_glob(const char *pattern, int flags, void *errfunc, glob_t *pglob)
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   if(!pattern) {
       return -1;
   }
   hFind = FindFirstFile(pattern, &FindFileData);
   if (hFind == INVALID_HANDLE_VALUE) {
       return -1;
   } else {
       char tmp[ARCHDEP_PATH_MAX];
       strcpy(tmp, pattern);
       char *del = strrchr(tmp, '/');
       char *fn = FindFileData.cFileName;
       if (del) {
           *del = 0;
           fh[0] = util_concat(tmp, "/", fn, NULL);
       } else {
           fh[0] = fn;
       }
       pglob->gl_pathc = 1;
       pglob->gl_offs = 1;
       pglob->gl_pathv = fh;
       FindClose(hFind);
   }
   return 0;
}

void archdep_globfree(glob_t *pglob)
{
    lib_free(pglob->gl_pathv[0]);
}

#endif
