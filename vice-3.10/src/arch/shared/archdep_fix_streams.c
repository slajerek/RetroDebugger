/** \file   archdep_fix_streams.c
 * \brief   Fix stdin, stdout, stderr streams if applicable
 *
 * Redirects stdin, stdout and stderr in case these are not available to the
 * running emulator. Specifically this means Windows compiled with the -mwindows
 * switch, causing stdin, stdout and stderr to be disconnected from the spawning
 * shell (cmd.exe, looks like msys2's shell already does this hack).
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * (With help from elgonzo to fix some issues, see
 * https://sourceforge.net/p/vice-emu/bugs/2065/#4eb6)
 *
 * OS support:
 *  - Windows
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

#include "archdep_fix_streams.h"

/** \fn archdep_fix_streams
 * \brief   Fix standard streams
 *
 * Make the standard streams `stdin`, `stdout` and `stderr` available to the
 * running application so we can log to the terminal, if any, using those
 * streams.
 *
 * \todo    Prompt doesn't reappear, need to press Enter in cmd.exe
 * \todo    Prompt reappears at odd location (middle of console) in case of
 *          PowerShell.
 * \todo    Piping (to `more`) doesn't work
 */

#ifdef WINDOWS_COMPILE

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>


void archdep_fix_streams(void)
{
    /* We build with -mconsole for debug builds, so the following code should
     * only run with --disable-debug (configure's default) */
#ifndef DEBUG
    const char *msystem = getenv("MSYSTEM");

    if (msystem == NULL || *msystem == '\0') {
        /* Not running from an MSYS shell: redirect stdout and stderr */
        FILE   *fp_out;
        FILE   *fp_err;
        HANDLE  handle;
        DWORD   filetype;
        BOOL    is_cons;

        handle   = GetStdHandle(STD_OUTPUT_HANDLE);
        filetype = GetFileType(handle);
        is_cons  = (filetype == FILE_TYPE_UNKNOWN || filetype == FILE_TYPE_CHAR);

        if (filetype == FILE_TYPE_DISK) {
            /* don't call AttachConsole in case of redirecting to file */
            return;
        }

        /* attach console to process for output redirection */
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            if (is_cons) {
                /* stdout is an interactive console */
                freopen_s(&fp_out, "CONOUT$", "wt", stdout);
                /* just redirect stderr as well, no idea if windows supports
                 * redirecting stdout but not stderr (or vice versa) */
                freopen_s(&fp_err, "CONOUT$", "wt", stderr);
            }
        }

        /* clean up */
        CloseHandle(handle);
    }
#endif  /* ifndef DEBUG */
}

#else   /* ifdef WINDOWS_COMPILE */
void archdep_fix_streams(void)
{
    /* Other OSes aren't retarded */
}
#endif  /* ifdef WINDOWS_COMPILE */
