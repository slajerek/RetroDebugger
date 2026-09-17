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
#include <glib.h>
#include <glib/gstdio.h>

#include "archdep.h"
#include "debug_gtk3.h"
#include "findpath.h"
#include "hotkeys.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "util.h"
#include "uiapi.h"


#if 0
/** \brief  Prefix used for autostart disk images
 */
#define AUTOSTART_FILENAME_PREFIX   "autostart-"


/** \brief  Suffix used for autostart disk images
 */
#define AUTOSTART_FILENAME_SUFFIX   ".d64"
#endif

/** \brief  Reference to argv[0]
 *
 * FIXME: this is only used twice I think, better pass this as an argument to
 *        the functions using it
 */
static char *argv0 = NULL;

/** \brief  Arch-dependent init
 *
 * \param[in]   argc    pointer to argument count
 * \param[in]   argv    argument vector
 *
 * \return  0
 */
int archdep_init(int *argc, char **argv)
{
#ifdef HAVE_DEBUG_GTK3UI
#if 0
    const char *prg_name;
    const char *config_path;
    const char *cache_path;
    const char *state_path;
    char       *searchpath;
    char       *vice_ini;
    char       *datadir;
    char       *docsdir;
# if defined(LINUX_COMPILE) || defined(BSD_COMPILE)
    char       *xdg_cache;
    char       *xdg_config;
    char       *xdg_data;
    char       *xdg_state;
# endif
#endif
#endif
    argv0 = lib_strdup(argv[0]);

    /* set argv0 for program_name()/boot_path() calls (yes, not ideal) */
    archdep_program_path_set_argv0(argv[0]);

    archdep_create_user_cache_dir();
    archdep_create_user_config_dir();
    archdep_create_user_state_dir();

#if 0
#ifdef HAVE_DEBUG_GTK3UI
    /* sanity checks, to remove later: */
    prg_name    = archdep_program_name();
    searchpath  = archdep_default_sysfile_pathlist(machine_name);
    config_path = archdep_user_config_path();
    cache_path  = archdep_user_cache_path();
    state_path  = archdep_user_state_path();
    vice_ini    = archdep_default_resource_file_name();
    datadir     = archdep_get_vice_datadir();
    docsdir     = archdep_get_vice_docsdir();

    debug_gtk3("program name    = \"%s\"", prg_name);
    debug_gtk3("user home dir   = \"%s\"", archdep_home_path());
    debug_gtk3("user cache dir  = \"%s\"", cache_path);
    debug_gtk3("user config dir = \"%s\"", config_path);
    debug_gtk3("user state dir  = \"%s\"", state_path);
    debug_gtk3("prg boot path   = \"%s\"", archdep_boot_path());
    debug_gtk3("VICE searchpath = \"%s\"", searchpath);
    debug_gtk3("VICE data path  = \"%s\"", datadir);
    debug_gtk3("VICE docs path  = \"%s\"", docsdir);
    debug_gtk3("vice.ini path   = \"%s\"", vice_ini);

# if defined(LINUX_COMPILE) || defined(BSD_COMPILE)
    xdg_cache = archdep_xdg_cache_home();
    xdg_config = archdep_xdg_config_home();
    xdg_data = archdep_xdg_data_home();
    xdg_state = archdep_xdg_state_home();

    debug_gtk3("XDG_CACHE_HOME  = '%s'.", xdg_cache);
    debug_gtk3("XDG_CONFIG_HOME = '%s'.", xdg_config);
    debug_gtk3("XDG_DATA_HOME   = '%s'.", xdg_data);
    debug_gtk3("XDG_STATE_HOME  = '%s'.", xdg_state);

    lib_free(xdg_cache);
    lib_free(xdg_config);
    lib_free(xdg_data);
    lib_free(xdg_state);
# endif

    lib_free(searchpath);
    lib_free(vice_ini);
# if 0
    lib_free(cfg_path);
# endif
    lib_free(datadir);
    lib_free(docsdir);
#endif
#endif
    /* needed for early log control (parses for -silent/-verbose) */
    log_early_init(*argc, argv);
#if 0
    debug_gtk3("MSYSTEM = '%s'", getenv("MSYSTEM"));
#endif

    return 0;
}


/** \brief  Architecture-dependent shutdown handler
 */
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

    /* this should be removed soon */
    if (argv0 != NULL) {
        lib_free(argv0);
        argv0 = NULL;
    }

#ifndef WINDOWS_COMPILE
    archdep_network_shutdown();
#endif
}
