/*
 * system.c - System dependant functions.
 *
 * Written by
 *  Markus Brenner <markus@brenner.de>
 *  Andreas Boose <viceteam@t-online.de>
 *
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

#include <windows.h>
#include <prsht.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "system.h"


void system_init_dialog(HWND hwnd)
{
}

void system_psh_settings(PROPSHEETHEADER* ppsh)
{
}

size_t system_wcstombs(char *mbs, const char *wcs, size_t len)
{
    strncpy(mbs, wcs, len);
    return strlen(mbs);
}

size_t system_mbstowcs(char *wcs, const char *mbs, size_t len)
{
    strncpy(wcs, mbs, len);
    return strlen(wcs);
}

char *system_mbstowcs_alloc(const char *mbs)
{
    char *wcs;

    if (mbs == NULL) {
        return NULL;
    }

    wcs = lib_malloc((strlen(mbs) + 1) * sizeof(char));
    system_mbstowcs(wcs, mbs, strlen(mbs) + 1);

    return wcs;
}

void system_mbstowcs_free(char *wcs)
{
    lib_free(wcs);
}

char *system_wcstombs_alloc(const char *wcs)
{
    char *mbs;

    if (wcs == NULL) {
        return NULL;
    }

    mbs = lib_malloc((strlen(wcs) + 1) * sizeof(char));
    system_wcstombs(mbs, wcs, strlen(wcs) + 1);

    return mbs;
}

void system_wcstombs_free(char *mbs)
{
    lib_free(mbs);
}
