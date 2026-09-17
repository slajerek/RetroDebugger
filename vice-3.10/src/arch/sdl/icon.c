/** \file   icon.c
 * \brief   SDL Window icon(s) support
 *
 * Sets application icon on Unix for SDL1.2 and SDL2 and on Windows for SDL1.2.
 *
 * \author  groepaz <groepaz@gmx.net>
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

#include "vice.h"

#include "archdep.h"
#include "archdep_defs.h"
#include "vice_sdl.h"
#ifndef USE_SDL_PREFIX
# include <SDL_image.h>
#else
# ifdef USE_SDL2UI
#  include <SDL2/SDL_image.h>
# else
#  include <SDL/SDL_image.h>
# endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lib.h"
#include "machine.h"

#include "icon.h"


#ifdef USE_SDL2UI

/** \brief  Set window icon
 *
 * \param[in]   window  window instance
 *
 * \note    requires libsdl2-image-dev on Linux, SDL2_image on msys2
 */
void sdl_ui_set_window_icon(SDL_Window *window)
{
    SDL_Surface *surface;
    char *path;

    path = archdep_app_icon_path_png(256);
    if (!path) {
        return;
    }

    IMG_Init(IMG_INIT_PNG);
    surface = IMG_Load(path);
    lib_free(path);

    /* The icon is attached to the window pointer */
    SDL_SetWindowIcon(window, surface);

    /* ...and the surface containing the icon pixel data is no longer required. */
    SDL_FreeSurface(surface);
}

#else


/** \brief  Set window icon
 *
 * \param[in]   window  window instance (unused)
 *
 * \note    requires libsdl-image1.2-dev on Linux, SDL_image on msys2
 * \note    needs to be called before the first call to SDL_SetVideoMode()
 */
void sdl_ui_set_window_icon(void *window)
{
    SDL_Surface *surface;
    char *path;

    /* SDL1.2 docs say Win32 icons need to be 32x32. On Win7 using 256x256
     * also works fine, but let's do what the docs say anyway.
     */
#ifdef WINDOWS_COMPILE
    path = archdep_app_icon_path_png(32);
#else
    path = archdep_app_icon_path_png(256);
#endif
    if (!path) {
        return;
    }

    IMG_Init(IMG_INIT_PNG);
    surface = IMG_Load(path);
    lib_free(path);

    SDL_WM_SetIcon(surface, NULL);
    SDL_FreeSurface(surface);
}

#endif
