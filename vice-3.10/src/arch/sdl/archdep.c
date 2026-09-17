/** \file   archdep.c
 * \brief   Miscellaneous system-specific stuff for SDL
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice_sdl.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "archdep.h"
#include "findpath.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "ui.h"
#include "util.h"

/* FIXME: includes for beos */
/* FIXME: includes for os/2 */

/* These functions are defined in the files included below and
   used lower down. */
static int archdep_init_extra(int *argc, char **argv);
static void archdep_shutdown_extra(void);

#include "kbd.h"
#include "log.h"

#ifndef SDL_REALINIT
#define SDL_REALINIT SDL_Init
#endif


/******************************************************************************/

#ifdef WINDOWS_COMPILE
/* for O_BINARY */
#include <fcntl.h>
#endif

/* called from archdep.c:archdep_init */
static int archdep_init_extra(int *argc, char **argv)
{
#if defined(WINDOWS_COMPILE)
    _fmode = O_BINARY;

    _setmode(_fileno(stdin), O_BINARY);
    _setmode(_fileno(stdout), O_BINARY);
#endif
    return 0;
}

/* called from archdep.c:archdep_shutdown */
static void archdep_shutdown_extra(void)
{
}

/******************************************************************************/

int archdep_init(int *argc, char **argv)
{
    archdep_program_path_set_argv0(argv[0]);

    archdep_create_user_cache_dir();
    archdep_create_user_config_dir();

    if (SDL_REALINIT(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        return 1;
    }

    /*
     * Call SDL_Quit() via atexit() to avoid segfaults on exit.
     * See: https://wiki.libsdl.org/SDL_Quit
     * I'm not sure this actually registers SDL_Quit() as the last atexit()
     * call, but it appears to work at least (BW)
     */
    if (atexit(SDL_Quit) != 0) {
        log_error(LOG_DEFAULT,
                "failed to register SDL_Quit() with atexit().");
        archdep_vice_exit(1);
    }

    return archdep_init_extra(argc, argv);
}

void archdep_shutdown(void)
{
    /* free memory used by the exec path */
    archdep_program_path_free();
    /* free memory used by the exec name */
    archdep_program_name_free();
    /* free memory used by the boot path */
    archdep_boot_path_free();
    /* free memory used by the home path */
    archdep_home_path_free();
    /* free memory used by the cache files path */
    archdep_user_cache_path_free();
    /* free memory used by the config files path */
    archdep_user_config_path_free();
    /* free memory used by the state files path */
    archdep_user_state_path_free();
    /* free memory used by the sysfile pathlist */
    archdep_default_sysfile_pathlist_free();

#ifdef HAVE_NETWORK
    archdep_network_shutdown();
#endif
    archdep_shutdown_extra();
    archdep_extra_title_text_free();
}
