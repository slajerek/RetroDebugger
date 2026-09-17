/*
 * uipause.c - Pause routines.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice_sdl.h"

#include "interrupt.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uimenu.h"
#include "vsync.h"

/* ----------------------------------------------------------------- */
/* ui.h */

static int is_paused = 0;

bool ui_pause_loop_iteration(void)
{
    ui_dispatch_events();
    SDL_Delay(10);

    return is_paused;
}

static void pause_trap(uint16_t addr, void *data)
{
    vsync_suspend_speed_eval();
    sound_suspend();

    while (ui_pause_loop_iteration());
}


/** \brief  Get current pause state
 *
 * \return  boolean
 */
int ui_pause_active(void)
{
    return is_paused;
}


/** \brief  Enable pause
 */
void ui_pause_enable(void)
{
    is_paused = 1;
    interrupt_maincpu_trigger_trap(pause_trap, 0);
}


/** \brief  Disable pause
 */
void ui_pause_disable(void)
{
    is_paused = 0;
}


/** \brief  Toggle pause
 */
void ui_pause_toggle(void)
{
    if (ui_pause_active()) {
        ui_pause_disable();
    } else {
        ui_pause_enable();
    }
}
