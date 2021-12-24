/*
 * uimon.c - Monitor access interface.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#include "console.h"
#include "lib.h"
#include "monitor.h"
#include "uimon.h"
#include "ui.h"
#include "uimenu.h"
#include "log.h"
#include "ViceWrapper.h"

static console_t mon_console = {
    40,
    25,
    0,
    0,
    NULL
};

static menu_draw_t *menu_draw = NULL;

static int x_pos = 0;

void uimon_window_close(void)
{
    sdl_ui_activate_post_action();
}

console_t c64d_console_log = { 80, 25, 1, 0 };

console_t *uimon_window_open(void)
{
	return &c64d_console_log;
	
//    sdl_ui_activate_pre_action();
//    sdl_ui_init_draw_params();
//    sdl_ui_clear();
//    menu_draw = sdl_ui_get_menu_param();
//    mon_console.console_xres = menu_draw->max_text_x;
//    mon_console.console_yres = menu_draw->max_text_y;
//    x_pos = 0;
//    return &mon_console;
}

void uimon_window_suspend(void)
{
    uimon_window_close();
}

console_t *uimon_window_resume(void)
{
    return uimon_window_open();
}

int uimon_out(const char *buffer)
{
//    int y = menu_draw->max_text_y - 1;
    char *p = (char *)buffer;
    int i = 0;
    char c;

	LOGD("uimon_out: %s", buffer);	

    while ((c = p[i]) != 0) {
        if (c == '\n') {
            p[i] = 0;
            //sdl_ui_print(p, x_pos, y);
            //sdl_ui_scroll_screen_up();
			c64d_uimon_print_line(p);

            //x_pos = 0;
            p += i + 1;
            i = 0;
        } else {
            ++i;
        }
    }

    if (p[0] != 0) {
        //x_pos += sdl_ui_print(p, x_pos, y);
		c64d_uimon_print(p);
    }
    return 0;
}

char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
    int y, x_off;
    char *input;

    y = menu_draw->max_text_y - 1;
    x_pos = 0;

    x_off = sdl_ui_print(prompt, 0, y);
    input = sdl_ui_readline(NULL, x_off, y);
    sdl_ui_scroll_screen_up();

    if (input == NULL) {
        input = lib_stralloc("x");
    }

    return input;
}

void uimon_notify_change(void)
{
    sdl_ui_refresh();
}

void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count)
{
}
