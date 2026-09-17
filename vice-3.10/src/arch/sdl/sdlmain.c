/*
 * sdlmain.c - SDL startup.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "log.h"
#include "machine.h"
#include "main.h"
#include "resources.h"
#include "uimenu.h"

#include "vice_sdl.h"

/* FIXME: Ugly hack for preventing SDL crash using -help */
int sdl_help_shutdown = 0;

int main(int argc, char **argv)
{
    return main_program(argc, argv);
}

void main_exit(void)
{
    /* FIXME: Ugly hack for preventing SDL crash using -help */
    if (!sdl_help_shutdown) {
        /* log resources with non default values */
        resources_log_active();
        /* log the active config as commandline options */
        cmdline_log_active();
    }

    log_message(LOG_DEFAULT, "\nExiting...");

    /*
     * Clean up dangling resources due to the 'Quit emu' callback not returning
     * to the calling menu code.
     */
    sdl_ui_menu_shutdown();

    machine_shutdown();

    putchar('\n');
}
