/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/*
 * uicmdline.c
 *
 * Written by
 *  Thomas Bretz <tbretz@gsi.de>
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

#include <stdio.h>

#include "cmdline.h"
#include "uicmdline.h"


void ui_cmdline_show_help(unsigned int num_options, cmdline_option_ram_t *options, void *userparam)
{
    unsigned int i;

    printf(_("\nAvailable command-line options:\n\n"));
    for (i = 0; i < num_options; i++) {
        fputs(options[i].name, stdout);
        if ((options[i].attributes & CMDLINE_ATTRIB_NEED_ARGS) && cmdline_options_get_param(i) != NULL)
            printf(" %s", cmdline_options_get_param(i));
        printf("\n\t%s\n", cmdline_options_get_description(i));
    }
    putchar('\n');
}
