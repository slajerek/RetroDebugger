/** \file   archdep_default_logger.c
 * \brief   Write to default frogger
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
#include <errno.h>

#ifdef WINDOWS_COMPILE
# include <windows.h>
# include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef UNIX_COMPILE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "lib.h"
#include "util.h"

#include "archdep_defs.h"
#include "archdep_stat.h"
#include "archdep_default_logger.h"


#ifdef WINDOWS_COMPILE

/** \brief  Write message to Windows debugger/logger
 *
 * \param[in]   level_string    log level string
 * \param[in]   txt             log message
 *
 * \note    Shamelessly copied from win32/archdep.c
 *
 * \return  0 on success, < 0 on failure
 */
int archdep_default_logger(const char *level_string, const char *txt)
{
    char *out;

    if (level_string != NULL && *level_string != '\0') {
        out = lib_msprintf("%s %s", level_string, txt);
    } else {
        out = lib_strdup(txt);
    }

    /* first output to stdout as usual, and flush the output so it
       shows up in the console immediately */
    puts(out);
    fflush(stdout);
    /* also output to debug output, so it can be captured by Debugview etc */
    OutputDebugString(out);

    lib_free(out);
    return 0;
}

int archdep_default_logger_is_terminal(void)
{
    struct stat statinfo;
    DWORD temp;
    const bool mode = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &temp);
    const int type = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
    /* this works in cmd.exe */
    if (mode || (type == FILE_TYPE_CHAR)) {
        return 1;
    }

    /* extra check seems to work for msys */
    fstat(fileno(stdout), &statinfo);
    /* 1 - 0    msys (-noredir)
       0 - 1    msys (-noredir) > bla
       0 - 1    cmd -noredir > bla
       0 - 0    cmd > bla
       0 - 0    cmd
    */
    if (S_ISFIFO(statinfo.st_mode) && !S_ISREG(statinfo.st_mode)) {
        return 1;
    }
    return 0;
}

#elif defined(UNIX_COMPILE)

/** \brief  Write log message to stdout
 *
 * \param[in]   level_string    log level string
 * \param[in]   txt             log message
 *
 * \note    Shamelessly copied from unix/archdep.c
 *
 * \return  0 on success, < 0 on failure
 */
int archdep_default_logger(const char *level_string, const char *txt)
{
    if (fputs(level_string, stdout) == EOF
            || fprintf(stdout, "%s", txt) < 0
            || fputc ('\n', stdout) == EOF) {
        return -1;
    }
    return 0;
}

int archdep_default_logger_is_terminal(void)
{
    FILE *fp = stdout;
    struct stat statinfo;

    fstat(fileno(fp), &statinfo);
    if (!S_ISFIFO(statinfo.st_mode) && !S_ISREG(statinfo.st_mode)) {
        return 1;
    }
    return 0;
}

#else

/** \brief  Write log message to stdout
 *
 * Stub: doesn't do anything but return 0.
 *
 * \param[in]   level_string    log level string
 * \param[in]   txt             log message
 *
 * \return  0
 */
int archdep_default_logger(const char *level_string, const char *txt)
{
    return 0;
}

int archdep_default_logger_is_terminal(void)
{
    return 0;   /* FIXME */
}

#endif
