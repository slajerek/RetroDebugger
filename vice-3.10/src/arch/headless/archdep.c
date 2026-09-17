/** \file   archdep.c
 * \brief   Wrappers for architecture/OS-specific code
 *
 * I've decided to use GLib's use of the XDG specification and the standard
 * way of using paths on Windows. So some files may not be where the older
 * ports expect them to be. For example, vicerc will be in $HOME/.config/vice
 * now, not $HOME/.vice. -- compyx
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"


/** \brief  Reference to argv[0]
 *
 * FIXME: this is only used twice I think, better pass this as an argument to
 *        the functions using it
 */
static char *argv0 = NULL;

/** \brief  Signal handler that initialises a clean shutdown.
 */
static void sigint_exit(int _)
{
    archdep_vice_exit(0);
}

/** \brief  Arch-dependent init
 *
 * \param[in]   argc    pointer to argument count
 * \param[in]   argv    argument vector
 *
 * \return  0
 */
int archdep_init(int *argc, char **argv)
{
    /* printf("%s\n", __func__); */

    /* Initiate a clean shutdown on CTRL-C */
    signal(SIGINT, sigint_exit);

    argv0 = lib_strdup(argv[0]);

    /* set argv0 for program_name()/boot_path() calls (yes, not ideal) */
    archdep_program_path_set_argv0(argv[0]);

    archdep_create_user_cache_dir();
    archdep_create_user_config_dir();

    /* needed for early log control (parses for -silent/-verbose) */
    log_early_init(*argc, argv);

    return 0;
}

/** \brief  Architecture-dependent shutdown hanlder
 */
void archdep_shutdown(void)
{
    /* printf("%s\n", __func__); */

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
    /* free memory used by the sysfile pathlist */
    archdep_default_sysfile_pathlist_free();

    /* this should be removed soon */
    if (argv0 != NULL) {
        lib_free(argv0);
        argv0 = NULL;
    }

#ifndef WINDOWS_COMPILE
    archdep_network_shutdown();
#endif
}

