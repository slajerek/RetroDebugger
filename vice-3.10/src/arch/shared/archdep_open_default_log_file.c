/** \file   archdep_open_default_log_file.c
 * \brief   Open default log file
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Open default log file `$XDG_STATE_HOME/vice/vice.log`, which defaults to
 * `$HOME/.local/state/vice/vice.log`.
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
#include "archdep_defs.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef UNIX_COMPILE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "archdep_default_logfile.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_open_default_log_file.h"


/** \brief  Opens the default log file
 *
 * Attempt to open a log file in the user's vice state dir. If the file cannot
 * be opened for some reason, return NULL.
 *
 * NOTE: In the past this function would return stdout (instead of opening a
 * regular file) under certain conditions, eg when stdout was connected to a
 * pipe or a terminal. Or simply when opening the default log file failed. This
 * is not the case anymore, since the log system handles the log file and stdout
 * separately now.
 *
 * \return  file pointer to log file, or NULL on failure.
 */
FILE *archdep_open_default_log_file(void)
{
    FILE *fp;
    char *path;

    path = archdep_default_logfile();
    fp = fopen(path, "w");
    if (fp == NULL) {
        log_error(LOG_DEFAULT,
                "failed to open log file '%s' for writing.",
                path);
    }
    lib_free(path);

    return fp;
}
