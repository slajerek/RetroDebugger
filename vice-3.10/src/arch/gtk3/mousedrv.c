/** \file   mousedrv.c
 * \brief   Native GTK3 UI mouse driver stuff.
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#if defined(MACOS_COMPILE)
#import <CoreGraphics/CGRemoteOperation.h>
#import <CoreGraphics/CGEvent.h>
#elif defined(WINDOWS_COMPILE)
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#endif


#include <stdio.h>
#include <math.h>

#include "log.h"
#include "vsyncapi.h"
#include "maincpu.h"
#include "mouse.h"
#include "mousedrv.h"
#include "ui.h"
#include "uimachinewindow.h"

static log_t mousedrv_log = LOG_DEFAULT;

/** \brief The callbacks registered for mouse buttons being pressed or
 *         released.
 *  \sa mousedrv_resources_init which sets these values properly
 *  \sa mouse_button which uses them
 */
static mouse_func_t mouse_funcs = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int mousedrv_cmdline_options_init(void)
{
    return 0;
}

void mouse_button(int bnumber, int state)
{
    switch(bnumber) {
    case 0:
        if (mouse_funcs.mbl) {
            mouse_funcs.mbl(state);
        }
        break;
    case 1:
        if (mouse_funcs.mbm) {
            mouse_funcs.mbm(state);
        }
        break;
    case 2:
        if (mouse_funcs.mbr) {
            mouse_funcs.mbr(state);
        }
        break;
    case 3:
        if (mouse_funcs.mbu) {
            mouse_funcs.mbu(state);
        }
        break;
    case 4:
        if (mouse_funcs.mbd) {
            mouse_funcs.mbd(state);
        }
        break;
    default:
        log_warning(mousedrv_log, "Strange mouse button %d\n", bnumber);
    }
}

void mousedrv_init(void)
{
    /* This does not require anything special to be done */
}

void mousedrv_mouse_changed(void)
{
    /** \todo Tell UI level to capture mouse cursor if necessary and
     *        permitted */
    log_verbose(mousedrv_log, "Status changed: %d (%s)",
            _mouse_enabled, _mouse_enabled ? "enabled" : "disabled");
    if (_mouse_enabled) {
        ui_mouse_grab_pointer();
    } else {
        ui_mouse_ungrab_pointer();
    }
}

int mousedrv_resources_init(const mouse_func_t *funcs)
{
    mousedrv_log = log_open("GTK3 Mouse");
    /* Copy entire 'mouse_func_t' structure. */
    mouse_funcs = *funcs;
    return 0;
}
