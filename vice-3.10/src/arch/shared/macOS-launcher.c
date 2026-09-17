/*
 * macOS-launcher.c - Native wrapper for the macOS launcher bash script
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

int main(int argc, char *argv[])
{
    /*
     * This exists to ensure that macOS doesn't think Rosetta 2 is needed
     * to run the bash script launcher. It simply finds the path to the
     * bash script (which is in the same directory as this executable) and
     * execs it with the same arguments.
     */

    char exe_path[PATH_MAX];
    char real_path[PATH_MAX];
    char script_path[PATH_MAX];
    char *exe_dirname;
    char *exe_filename;
    uint32_t size = sizeof(exe_path);

    /* Get the path to this executable \*/
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        fprintf(stderr, "Error: Could not determine executable path\n");
        return 1;
    }

    /* Resolve symlinks */
    if (realpath(exe_path, real_path) == NULL) {
        fprintf(stderr, "Error: Could not resolve executable path\n");
        return 1;
    }

    /* Split into dirname and basename */
    exe_dirname  = dirname(real_path);
    exe_filename = basename(real_path);

    /* Build path to the bash script (same directory as this executable) */
    snprintf(script_path, sizeof(script_path), "%s/../Resources/%s.sh", exe_dirname, exe_filename);

    /* Replace argv[0] with the script path */
    argv[0] = script_path;

    /* Execute the bash script */
    execv(script_path, argv);

    /* If we get here, execv failed */
    perror("Error executing launcher script");
    return 1;
}
